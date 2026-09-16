using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using Ming;

// The engine string wrapper shares its name with System.String once Ming is imported.
// e.g. String* parameters below are Ming.String, while string stays System.String.
using String = Ming.String;

namespace MingPlugins
{
    public static class Main
    {
        // Keep a strong reference to the context only through this holder. Marking every
        // member that touches it as non-inlineable stops the JIT from leaving a stray
        // reference alive on the stack, which would prevent the context from unloading.
        private sealed class ProjectLoadContextHolder
        {
            private PluginLoadContext? _context;
            private readonly WeakReference _weakReference;
            public bool IsLoaded { get; set; }

            private ProjectLoadContextHolder(PluginLoadContext context, WeakReference weakReference)
            {
                _context = context;
                _weakReference = weakReference;
            }

            public string AssemblyLoadedPath
            {
                [MethodImpl(MethodImplOptions.NoInlining)]
                get => _context?.AssemblyLoadedPath ?? string.Empty;
            }

            public bool IsAlive
            {
                [MethodImpl(MethodImplOptions.NoInlining)]
                get => _weakReference.IsAlive;
            }

            [MethodImpl(MethodImplOptions.NoInlining)]
            public static (Assembly, ProjectLoadContextHolder) CreateAndLoad(
                string pluginPath,
                IEnumerable<string> sharedAssemblies,
                AssemblyLoadContext hostLoadContext)
            {
                var context = new PluginLoadContext(pluginPath, sharedAssemblies, hostLoadContext);
                var weakReference = new WeakReference(context, trackResurrection: true);
                var holder = new ProjectLoadContextHolder(context, weakReference);
                s_projectLoadContext = holder;
                var assembly = context.LoadPlugin(pluginPath);
                return (assembly, holder);
            }

            [MethodImpl(MethodImplOptions.NoInlining)]
            public void Unload()
            {
                IsLoaded = false;
                PluginLoadContext? context = _context;
                _context = null;
                context?.Unload();
            }
        }

        // Assemblies shared with the project assembly so types such as MingObject resolve
        // to a single instance.
        private static readonly List<string> s_sharedAssemblies = new();

        // The context that owns the shared assemblies: the one MingPlugins itself lives in.
        private static readonly AssemblyLoadContext s_hostLoadContext =
            AssemblyLoadContext.GetLoadContext(typeof(MingObject).Assembly) ?? AssemblyLoadContext.Default;

        private static ProjectLoadContextHolder? s_projectLoadContext;

        private static int Log(string message) => NativeFuncs.Log(message);

        [UnmanagedCallersOnly]
        private static unsafe int Initialize(
            void* nativeCallbacks,
            int nativeCallbacksSize,
            void* managedCallbacks,
            int managedCallbacksSize
        )
        {
            try
            {
                if (nativeCallbacks == null)
                {
                    return -1;
                }

                #region Initialize native and managed callbacks

                NativeFuncs.Initialize(
                    (IntPtr)nativeCallbacks,
                    nativeCallbacksSize,
                    (IntPtr)managedCallbacks,
                    managedCallbacksSize
                );

                // The engine API assembly must be shared so the project assembly sees the
                // very same types (MingObject, Vector3, ...) instead of its own copies.
                s_sharedAssemblies.Add(typeof(MingObject).Assembly.GetName().Name!);

                #endregion

                return 0;
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine(exception);
                return -5;
            }
        }

        [UnmanagedCallersOnly]
        private static unsafe int Shutdown()
        {
            try
            {
                return Log(ScriptManagerBridge.GetBindingSummary());
            }
            catch (Exception)
            {
                return -1;
            }
        }

        [UnmanagedCallersOnly]
        private static unsafe int LoadProjectAssembly(String* assemblyPath, String* outLoadedAssemblyPath)
        {
            if (outLoadedAssemblyPath == null)
            {
                return -1;
            }

            *outLoadedAssemblyPath = default;

            if (assemblyPath == null)
            {
                return -1;
            }

            string path = string.Empty;

            try
            {
                // 1) Convert and validate the absolute path
                path = Marshaling.ConvertStringToManaged(*assemblyPath);

                if (string.IsNullOrWhiteSpace(path) || !Path.IsPathFullyQualified(path))
                {
                    Log($"[Error] Invalid project assembly path: '{path}'. Expected an absolute path.");
                    return -2;
                }

                path = Path.GetFullPath(path);

                // 2) Check the target file
                FileAttributes attributes;

                try
                {
                    attributes = File.GetAttributes(path);
                }
                catch (FileNotFoundException)
                {
                    Log($"[Error] Project assembly does not exist: {path}");
                    return 1;
                }
                catch (DirectoryNotFoundException)
                {
                    Log($"[Error] Project assembly does not exist: {path}");
                    return 1;
                }

                if ((attributes & FileAttributes.Directory) != 0)
                {
                    Log($"[Error] Project assembly path points to a directory: {path}");
                    return -2;
                }

                // 3) Skip if the project assembly is already loaded
                if (s_projectLoadContext != null)
                {
                    if (!s_projectLoadContext.IsLoaded)
                    {
                        Log("[Error] Previous project context must finish unloading before loading again.");
                        return -3;
                    }
                    *outLoadedAssemblyPath = Marshaling.ConvertStringToNative(
                        s_projectLoadContext.AssemblyLoadedPath);
                    return 0;
                }

                // 4) Load the project assembly into its own collectible context
                (Assembly projectAssembly, s_projectLoadContext) = ProjectLoadContextHolder.CreateAndLoad(
                    path,
                    s_sharedAssemblies,
                    s_hostLoadContext);

                string loadedPath = s_projectLoadContext.AssemblyLoadedPath;

                if (string.IsNullOrWhiteSpace(loadedPath))
                {
                    throw new InvalidOperationException(
                        "The loaded project assembly has no file location."
                    );
                }

                ScriptManagerBridge.LookupScriptsInAssembly(projectAssembly);

                // 5) Transfer the output string ownership to C++
                *outLoadedAssemblyPath = Marshaling.ConvertStringToNative(loadedPath);
                s_projectLoadContext.IsLoaded = true;

                return 0;
            }
            catch (Exception exception)
            {
                // 6) Clear partial registrations and retain the holder for unload verification
                try
                {
                    s_projectLoadContext?.Unload();
                }
                catch (Exception cleanupException)
                {
                    Console.Error.WriteLine(cleanupException);
                }

                // 7) Report failure without propagating logging exceptions
                try
                {
                    Log($"[Error] Failed to load project assembly: {path}\n{exception}");
                }
                catch
                {
                }

                return -5;
            }
        }

        [UnmanagedCallersOnly]
        private static unsafe int UnloadProjectAssembly()
        {
            try
            {
                // 1) Nothing to unload
                if (s_projectLoadContext == null)
                {
                    return 0;
                }

                Log("Unloading project assembly...");

                // 2) Start unloading; the context is only reclaimed once nothing
                //    references it anymore.
                s_projectLoadContext.Unload();

                // 3) Wait for the garbage collector to actually reclaim it
                int startTimeMs = Environment.TickCount;

                while (s_projectLoadContext.IsAlive)
                {
                    GC.Collect(GC.MaxGeneration, GCCollectionMode.Forced);
                    GC.WaitForPendingFinalizers();

                    if (!s_projectLoadContext.IsAlive)
                    {
                        break;
                    }

                    int elapsedTimeMs = Environment.TickCount - startTimeMs;

                    if (elapsedTimeMs >= 1000)
                    {
                        Log("[Error] Failed to unload the project assembly. "
                            + "Possible causes: strong GC handles, running threads, static references.");
                        return -1;
                    }
                }

                // 4) Forget the holder so the next load creates a fresh context
                s_projectLoadContext = null;

                Log("Project assembly unloaded.");
                return 0;
            }
            catch (Exception exception)
            {
                // 5) Report failure without propagating logging exceptions
                try
                {
                    Log($"[Error] Failed to unload the project assembly:\n{exception}");
                }
                catch
                {
                }

                return -5;
            }
        }

        [UnmanagedCallersOnly]
        private static unsafe int BuildProjectSolution(String* projectDirectory)
        {
            try
            {
                // 1) Validate and convert the native project path
                if (projectDirectory == null)
                {
                    return -1;
                }

                string projectPath = Marshaling.ConvertStringToManaged(*projectDirectory);

                // 2) Build the project and forward its diagnostics
                return MingTools.BuildSystem.BuildProjectSolution(projectPath, message => Log(message));
            }
            catch (Exception exception)
            {
                // 3) Report the error without crossing the native boundary
                try
                {
                    Log($"[Error] Failed to build C# project solution:\n{exception}");
                }
                catch
                {
                }

                return -5;
            }
        }

        [UnmanagedCallersOnly]
        private static unsafe int EnsureProjectSolution(String* projectDirectory, String* sdkDirectory)
        {
            try
            {
                // 1) Validate the native pointers
                if (projectDirectory == null || sdkDirectory == null)
                {
                    return -1;
                }

                // 2) Convert the native strings into managed strings
                string projectPath = Marshaling.ConvertStringToManaged(*projectDirectory);
                string sdkPath = Marshaling.ConvertStringToManaged(*sdkDirectory);

                // 3) Generate the missing project files
                MingTools.ProjectGenerator.EnsureProjectSolution(projectPath, sdkPath);

                return 0;
            }
            catch (Exception exception)
            {
                // 4) Report the error without crossing the native boundary
                try
                {
                    Log($"[Error] Failed to ensure C# project files:\n{exception}");
                }
                catch
                {
                }

                return -5;
            }
        }
    }
}
