using System.Diagnostics;
using System.Text;

namespace MingTools;

public static class BuildSystem
{
    public static int BuildProjectSolution(string projectDirectory, Action<string> log)
    {
        // 1) Validate the current project files
        ArgumentNullException.ThrowIfNull(log);

        if (string.IsNullOrWhiteSpace(projectDirectory)
            || !Path.IsPathFullyQualified(projectDirectory))
        {
            throw new ArgumentException(
                "Project directory must be an absolute path.",
                nameof(projectDirectory));
        }

        projectDirectory = Path.GetFullPath(projectDirectory);
        string solutionPath = Path.Combine(projectDirectory, "Game.sln");
        string projectPath = Path.Combine(projectDirectory, "Game.csproj");

        foreach (string path in new[] { solutionPath, projectPath })
        {
            if (!File.Exists(path))
            {
                throw new FileNotFoundException(
                    "C# project files are missing. Generate the project solution first.", path);
            }
        }

        // 2) Configure the Debug build process
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

        log($"Running: dotnet build \"{solutionPath}\" -c Debug --verbosity minimal");

        // 3) Drain both output streams while the process runs
        using var process = new Process { StartInfo = startInfo };
        process.Start();

        Task<string> standardOutput = process.StandardOutput.ReadToEndAsync();
        Task<string> standardError = process.StandardError.ReadToEndAsync();
        
        process.WaitForExit();

        // 4) Forward the build output on the calling thread
        foreach (string output in new[] {
            standardOutput.GetAwaiter().GetResult(),
            standardError.GetAwaiter().GetResult() })
        {
            using var reader = new StringReader(output);
            while (reader.ReadLine() is string line)
            {
                log(line);
            }
        }

        int exitCode = process.ExitCode;
        log($"C# build exited with code {exitCode}.");

        if (exitCode != 0)
        {
            return exitCode;
        }

        // 5) Verify the assembly expected by the engine
        string assemblyPath = Path.Combine(projectDirectory, ".ming", "dotnet", "bin", "Debug", "Game.dll");
        if (!File.Exists(assemblyPath))
        {
            throw new FileNotFoundException(
                "C# build succeeded but the expected project assembly was not found.", assemblyPath);
        }

        log($"C# project assembly built: {assemblyPath}");
        return 0;
    }
}
