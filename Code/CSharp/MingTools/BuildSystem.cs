using System.Collections.Concurrent;
using System.Diagnostics;
using System.Text;

namespace MingTools;

public static class BuildSystem
{
    public enum BuildState
    {
        Idle,
        Running,
        Succeeded,
        Failed
    }

    private static readonly ConcurrentQueue<string> s_output = new();
    private static Process? s_process;
    private static CancellationTokenSource? s_readCancellation;
    private static Task? s_standardOutput;
    private static Task? s_standardError;
    private static string s_assemblyPath = string.Empty;
    private static BuildState s_state;
    private static int s_exitCode = -1;

    public static bool BeginProjectBuild(string projectDirectory)
    {
        if (s_state == BuildState.Running)
        {
            return false;
        }

        CloseProjectBuild();
        s_state = BuildState.Running;
        try
        {
            // 1) Validate the current project files
            if (string.IsNullOrWhiteSpace(projectDirectory) || !Path.IsPathFullyQualified(projectDirectory))
            {
                throw new ArgumentException("Project directory must be an absolute path.", nameof(projectDirectory));
            }

            projectDirectory = Path.GetFullPath(projectDirectory);
            string solutionPath = Path.Combine(projectDirectory, "Game.sln");
            foreach (string path in new[] { solutionPath, Path.Combine(projectDirectory, "Game.csproj") })
            {
                if (!File.Exists(path))
                {
                    throw new FileNotFoundException("Generate the C# project solution before building.", path);
                }
            }

            s_assemblyPath = Path.Combine(projectDirectory, ".ming", "dotnet", "bin", "Debug", "Game.dll");

            // 2) Start the incremental Debug build without a shell window
            var startInfo = new ProcessStartInfo("dotnet")
            {
                WorkingDirectory = projectDirectory,
                UseShellExecute = false,
                CreateNoWindow = true,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                StandardOutputEncoding = Encoding.UTF8,
                StandardErrorEncoding = Encoding.UTF8
            };
            startInfo.ArgumentList.Add("build");
            startInfo.ArgumentList.Add(solutionPath);
            startInfo.ArgumentList.Add("-c");
            startInfo.ArgumentList.Add("Debug");
            startInfo.ArgumentList.Add("--verbosity");
            startInfo.ArgumentList.Add("minimal");
            s_output.Enqueue($"Running incremental C# build: {solutionPath}");

            var process = new Process { StartInfo = startInfo };
            try
            {
                process.Start();
            }
            catch
            {
                process.Dispose();
                throw;
            }

            s_process = process;
            s_readCancellation = new CancellationTokenSource();
            s_standardOutput = ReadOutput(process.StandardOutput, s_readCancellation.Token);
            s_standardError = ReadOutput(process.StandardError, s_readCancellation.Token);
            return true;
        }
        catch (Exception exception)
        {
            s_output.Enqueue($"[Error] Could not start C# build: {exception.Message}");
            s_state = BuildState.Failed;
            return false;
        }
    }

    public static BuildState PollProjectBuild(Action<string> log, out int exitCode)
    {
        ArgumentNullException.ThrowIfNull(log);

        // 1) Finish only after the process and both output readers have completed
        if (s_state == BuildState.Running && s_process != null && s_process.HasExited
            && s_standardOutput!.IsCompleted && s_standardError!.IsCompleted)
        {
            s_exitCode = s_process.ExitCode;
            bool succeeded = s_exitCode == 0;
            try
            {
                s_standardOutput.GetAwaiter().GetResult();
                s_standardError.GetAwaiter().GetResult();
                if (s_exitCode == 0 && !File.Exists(s_assemblyPath))
                {
                    throw new FileNotFoundException("C# build succeeded but Game.dll was not found.", s_assemblyPath);
                }
            }
            catch (Exception exception)
            {
                s_output.Enqueue($"[Error] C# build failed: {exception.Message}");
                succeeded = false;
            }

            s_state = succeeded ? BuildState.Succeeded : BuildState.Failed;
            s_output.Enqueue($"C# build exited with code {s_exitCode}.");
            s_process.Dispose();
            s_process = null;
            s_readCancellation?.Dispose();
            s_readCancellation = null;
            s_standardOutput = null;
            s_standardError = null;
        }

        // 2) Forward a bounded batch on the calling thread and drain before publishing completion
        for (int count = 0; count < 256 && s_output.TryDequeue(out string? line); ++count)
        {
            log(line);
        }

        exitCode = s_exitCode;
        return !s_output.IsEmpty ? BuildState.Running : s_state;
    }

    public static void CloseProjectBuild()
    {
        try
        {
            if (s_process != null && !s_process.HasExited)
            {
                s_process.Kill(entireProcessTree: true);
                s_process.WaitForExit();
            }
        }
        finally
        {
            s_readCancellation?.Cancel();
            try
            {
                Task.WhenAll(s_standardOutput ?? Task.CompletedTask, s_standardError ?? Task.CompletedTask)
                    .GetAwaiter().GetResult();
            }
            catch (Exception)
            {
                // 1) Discard output-reader failures during shutdown
            }

            s_process?.Dispose();
            s_readCancellation?.Dispose();
            s_process = null;
            s_readCancellation = null;
            s_standardOutput = null;
            s_standardError = null;
            s_output.Clear();
            s_state = BuildState.Idle;
            s_exitCode = -1;
        }
    }

    public static int BuildProjectSolution(string projectDirectory, Action<string> log)
    {
        ArgumentNullException.ThrowIfNull(log);
        if (s_state == BuildState.Running)
        {
            log("[Error] A C# build is already running.");
            return -1;
        }

        BeginProjectBuild(projectDirectory);
        try
        {
            while (PollProjectBuild(log, out _) == BuildState.Running)
            {
                Thread.Sleep(10);
            }

            return s_state == BuildState.Succeeded ? 0 : s_exitCode != 0 ? s_exitCode : -1;
        }
        finally
        {
            CloseProjectBuild();
        }
    }

    private static async Task ReadOutput(StreamReader reader, CancellationToken cancellationToken)
    {
        while (await reader.ReadLineAsync(cancellationToken).ConfigureAwait(false) is string line)
        {
            s_output.Enqueue(line);
        }
    }
}
