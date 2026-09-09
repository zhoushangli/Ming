using System;
using System.IO;
using System.Text;
using System.Xml.Linq;

namespace MingTools;

public static class ProjectGenerator
{
    public static void EnsureProjectSolution(string projectDirectory, string sdkDirectory)
    {
        // 1) Validate the input paths
        if (string.IsNullOrWhiteSpace(projectDirectory)
            || !Path.IsPathFullyQualified(projectDirectory))
        {
            throw new ArgumentException(
                "Project directory must be an absolute path.",
                nameof(projectDirectory));
        }

        if (string.IsNullOrWhiteSpace(sdkDirectory)
            || !Path.IsPathFullyQualified(sdkDirectory))
        {
            throw new ArgumentException(
                "SDK directory must be an absolute path.",
                nameof(sdkDirectory));
        }

        projectDirectory = Path.GetFullPath(projectDirectory);
        sdkDirectory = Path.GetFullPath(sdkDirectory);

        if (!Directory.Exists(projectDirectory))
        {
            throw new DirectoryNotFoundException(
                $"Project directory does not exist: {projectDirectory}");
        }

        // 2) Validate the SDK files
        string sdkPropsPath = Path.Combine(sdkDirectory, "Sdk.props");
        string sdkTargetsPath = Path.Combine(sdkDirectory, "Sdk.targets");

        if (!File.Exists(sdkPropsPath))
        {
            throw new FileNotFoundException(
                "Ming SDK props file was not found.",
                sdkPropsPath);
        }

        if (!File.Exists(sdkTargetsPath))
        {
            throw new FileNotFoundException(
                "Ming SDK targets file was not found.",
                sdkTargetsPath);
        }

        // 3) Create the missing project file
        string projectPath = Path.Combine(projectDirectory, "Game.csproj");

        if (!File.Exists(projectPath))
        {
            GenerateGameProject(projectPath, sdkDirectory);
        }

        // 4) Create the missing solution file
        string solutionPath = Path.Combine(projectDirectory, "Game.sln");

        if (!File.Exists(solutionPath))
        {
            GenerateGameSolution(solutionPath);
        }
    }

    private static void GenerateGameProject(
    string projectPath,
    string sdkDirectory)
    {
        // 1) Create the project XML
        var document = new XDocument(
            new XElement("Project",
                new XElement("PropertyGroup",
                    new XElement("MingSdkDirectory",
                        new XAttribute(
                            "Condition",
                            "'$(MingSdkDirectory)' == ''"),
                        sdkDirectory)),

                new XElement("Import",
                    new XAttribute(
                        "Project",
                        "$(MingSdkDirectory)/Sdk.props")),

                new XElement("PropertyGroup",
                    new XElement("TargetFramework", "net10.0"),
                    new XElement("OutputType", "Library"),
                    new XElement("AssemblyName", "Game"),
                    new XElement("RootNamespace", "Game")),

                new XElement("Import",
                    new XAttribute(
                        "Project",
                        "$(MingSdkDirectory)/Sdk.targets"))
            )
        );

        // 2) Write the new file without overwriting an existing file
        using var stream = new FileStream(
            projectPath,
            FileMode.CreateNew,
            FileAccess.Write);

        using var writer = new StreamWriter(
            stream,
            new UTF8Encoding(false));

        document.Save(writer);
    }

    private static void GenerateGameSolution(string solutionPath)
    {
        // 1) Create the solution project identifier
        string projectGuid = Guid.NewGuid()
            .ToString("B")
            .ToUpperInvariant();

        // 2) Create the single-project solution
        string solutionText = $$"""
        Microsoft Visual Studio Solution File, Format Version 12.00
        # Visual Studio Version 17
        Project("{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}") = "Game", "Game.csproj", "{{projectGuid}}"
        EndProject
        Global
            GlobalSection(SolutionConfigurationPlatforms) = preSolution
                Debug|Any CPU = Debug|Any CPU
                Release|Any CPU = Release|Any CPU
            EndGlobalSection
            GlobalSection(ProjectConfigurationPlatforms) = postSolution
                {{projectGuid}}.Debug|Any CPU.ActiveCfg = Debug|Any CPU
                {{projectGuid}}.Debug|Any CPU.Build.0 = Debug|Any CPU
                {{projectGuid}}.Release|Any CPU.ActiveCfg = Release|Any CPU
                {{projectGuid}}.Release|Any CPU.Build.0 = Release|Any CPU
            EndGlobalSection
        EndGlobal
        """;

        // 3) Write the new solution file
        using var stream = new FileStream(
            solutionPath,
            FileMode.CreateNew,
            FileAccess.Write);

        using var writer = new StreamWriter(
            stream,
            new UTF8Encoding(false));

        writer.WriteLine(solutionText);
    }
}

