using System;
using System.IO;
using Ming;

public partial class ManagedLifecycleProbe : Node
{
    public override void _EnterTree()
    {
        Record("E");
    }

    public override void _Ready()
    {
        Record("R");
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

    private static void Record(string value)
    {
        File.AppendAllText(Environment.GetEnvironmentVariable("MING_VERIFY_LIFECYCLE_LOG")!, value);
    }
}
