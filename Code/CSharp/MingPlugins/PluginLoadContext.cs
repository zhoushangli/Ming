namespace MingPlugins;

using System.Reflection;
using System.Runtime.Loader;

// Load the project assembly into its own collectible context so it can be unloaded
// and replaced while the engine keeps running.
// e.g. new PluginLoadContext(".../Game.dll", new[] { "MingSharp" }, hostContext)
public sealed class PluginLoadContext : AssemblyLoadContext
{
    private readonly AssemblyLoadContext _hostLoadContext;
    private readonly HashSet<string> _sharedAssemblies;
    private readonly string _pluginDirectory;

    // Absolute path of the project assembly that was loaded into this context.
    // e.g. ".../Project/.ming/dotnet/bin/Debug/Game.dll"
    public string AssemblyLoadedPath { get; private set; } = string.Empty;

    public PluginLoadContext(
        string pluginPath,
        IEnumerable<string> sharedAssemblies,
        AssemblyLoadContext hostLoadContext)
        : base(name: "MingProject", isCollectible: true)
    {
        _hostLoadContext = hostLoadContext;
        _sharedAssemblies = new HashSet<string>(sharedAssemblies, StringComparer.Ordinal);
        _pluginDirectory = Path.GetDirectoryName(Path.GetFullPath(pluginPath)) ?? string.Empty;
    }

    // Load the entry assembly and remember its path for diagnostics.
    // e.g. context.LoadPlugin(".../Game.dll") -> Assembly
    public Assembly LoadPlugin(string pluginPath)
    {
        AssemblyLoadedPath = Path.GetFullPath(pluginPath);
        return LoadFromCopy(AssemblyLoadedPath);
    }

    protected override Assembly? Load(AssemblyName assemblyName)
    {
        if (assemblyName.Name is not { } name)
        {
            return null;
        }

        // 1) Engine API assemblies must stay in the host context. Returning the very
        //    same Assembly instance is what keeps type identity intact across the boundary.
        if (_sharedAssemblies.Contains(name))
        {
            return _hostLoadContext.LoadFromAssemblyName(assemblyName);
        }

        // 2) Project-private dependency: look for it next to the project assembly.
        string candidatePath = Path.Combine(_pluginDirectory, name + ".dll");

        if (!File.Exists(candidatePath))
        {
            // 3) Not ours: let the runtime fall back to its default rules.
            return null;
        }

        return LoadFromCopy(candidatePath);
    }

    // Load an assembly from a memory copy so the file on disk stays writable.
    // e.g. LoadFromCopy(".../Game.dll") -> Assembly
    private Assembly LoadFromCopy(string assemblyPath)
    {
        string pdbPath = Path.ChangeExtension(assemblyPath, ".pdb");

        using FileStream assemblyFile = File.Open(
            assemblyPath,
            FileMode.Open,
            FileAccess.Read,
            FileShare.Read);

        if (File.Exists(pdbPath))
        {
            using FileStream pdbFile = File.Open(
                pdbPath,
                FileMode.Open,
                FileAccess.Read,
                FileShare.Read);

            return LoadFromStream(assemblyFile, pdbFile);
        }

        return LoadFromStream(assemblyFile);
    }
}