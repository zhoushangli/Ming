using System;
using System.IO;
using System.Threading;
using Ming;

public partial class ManagedLifecycleProbe : Node
{
    public override void _EnterTree()
    {
        Record("E");
    }

    public override void _Ready()
    {
        VerifyWorkerPool();
        Record("R");

        string path = Environment.GetEnvironmentVariable("MING_VERIFY_LIFECYCLE_LOG")! + ".worker";
        using var pending = Callable.From(() =>
        {
            Thread.Sleep(100);
            File.WriteAllText(path, "drained");
        });
        Require(WorkerThreadPool.AddTask(pending) >= 0, "unclaimed task submitted");
    }

    public override void _Process(float deltaSeconds)
    {
        if (deltaSeconds != GetSceneTree().GetDeltaSeconds())
        {
            throw new InvalidOperationException("Incorrect ordinary delta.");
        }

        Record("P");
    }

    public override void _PhysicsProcess(float deltaSeconds)
    {
        if (deltaSeconds != GetSceneTree().GetPhysicsDeltaSeconds())
        {
            throw new InvalidOperationException("Incorrect physics delta.");
        }

        Record("F");
    }

    public override void _ExitTree()
    {
        Record("X");
    }

    private static void VerifyWorkerPool()
    {
        int mainThread = Environment.CurrentManagedThreadId;
        int workerThread = mainThread;
        int result = 0;
        using var callback = Callable.From(() =>
        {
            workerThread = Environment.CurrentManagedThreadId;
            result = 42;
        });
        long task = WorkerThreadPool.AddTask(callback);
        callback.Dispose();
        Require(task >= 0, "ordinary task submitted");
        Require(!WorkerThreadPool.WaitForTaskCompletion(task + (1L << 32)), "binding preserves 64-bit IDs");
        Require(WorkerThreadPool.WaitForTaskCompletion(task) && result == 42 && mainThread != workerThread,
            "worker retains and executes a disposed wrapper's delegate");
        Require(!WorkerThreadPool.WaitForTaskCompletion(task) && !WorkerThreadPool.IsTaskCompleted(task),
            "ordinary task reclaimed once");

        int[] counts = new int[32];
        using var indexed = Callable.From<int>(index => { Interlocked.Increment(ref counts[index]); });
        Require(WorkerThreadPool.AddTask(indexed) == -1, "wrong callback signature rejected");
        Require(WorkerThreadPool.AddGroupTask(indexed, counts.Length, 0) == -1, "invalid task count rejected");

        long emptyGroup = WorkerThreadPool.AddGroupTask(indexed, 0, -1);
        Require(WorkerThreadPool.IsGroupTaskCompleted(emptyGroup) && WorkerThreadPool.WaitForGroupTaskCompletion(emptyGroup),
            "empty group is immediately reclaimable");
        long group = WorkerThreadPool.AddGroupTask(indexed, counts.Length, 4);
        indexed.Dispose();
        Require(WorkerThreadPool.WaitForGroupTaskCompletion(group) && !WorkerThreadPool.IsGroupTaskCompleted(group),
            "group reclaimed once");
        foreach (int count in counts)
        {
            Require(count == 1, "each index ran once");
        }

        using var throwing = Callable.From(() => { throw new InvalidOperationException("expected worker test exception"); });
        Require(WorkerThreadPool.WaitForTaskCompletion(WorkerThreadPool.AddTask(throwing)), "exception still completes task");

        using var named = new Callable(WorkerThreadPool.Singleton, nameof(WorkerThreadPool.AddTask));
        using var dynamicCallback = Callable.From(() => { result = 73; });
        using var argument = Variant.From(dynamicCallback);
        using var copy = argument.Clone();
        using var restoredCallback = copy.As<Callable>();
        using var restored = Variant.From(restoredCallback);
        using var dynamicTask = named.Call(new[] { restored }, out var error);
        Require(error.Error == CallErrorCode.Ok && WorkerThreadPool.WaitForTaskCompletion(dynamicTask.As<long>())
            && result == 73, "Variant MethodBind retains Callable and returns a long ID");
    }

    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException("Worker pool: " + message);
        }
    }

    private static void Record(string value)
    {
        File.AppendAllText(Environment.GetEnvironmentVariable("MING_VERIFY_LIFECYCLE_LOG")!, value);
    }
}
