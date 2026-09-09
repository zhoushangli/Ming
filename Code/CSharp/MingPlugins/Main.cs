using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Text;
using Ming;

namespace MingPlugins
{
    public static class Main
    {
        private static unsafe int Log(string message)
        {
            byte[] utf8Bytes = Encoding.UTF8.GetBytes(message);

            fixed (byte* text = utf8Bytes)
            {
                return NativeFuncs.LogUtf8(text, utf8Bytes.Length);
            }
        }

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
                return Log(NativeFuncs.GetBindingSummary());
            }
            catch (Exception)
            {
                return -1;
            }
        }

        [UnmanagedCallersOnly]
        private static unsafe int LoadProjectAssembly(char* assemblyPath, MingString* outLoadedAssemblyPath)
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
                // 1) Copy and validate the absolute path
                path = new string(assemblyPath);

                if (string.IsNullOrWhiteSpace(path) || !Path.IsPathFullyQualified(path))
                {
                    Log($"Invalid project assembly path: '{path}'. Expected an absolute path.");
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
                    Log($"Project assembly does not exist: {path}");
                    return 1;
                }
                catch (DirectoryNotFoundException)
                {
                    Log($"Project assembly does not exist: {path}");
                    return 1;
                }

                if ((attributes & FileAttributes.Directory) != 0)
                {
                    Log($"Project assembly path points to a directory: {path}");
                    return -2;
                }

                // 3) Get the MingSharp assembly load context
                var loadContext = AssemblyLoadContext.GetLoadContext(
                    typeof(MingObject).Assembly
                ) ?? throw new InvalidOperationException(
                    "Cannot find the MingSharp assembly load context."
                );

                // 4) Load the project assembly
                var projectAssembly = loadContext.LoadFromAssemblyPath(path);
                string loadedPath = projectAssembly.Location;

                if (string.IsNullOrWhiteSpace(loadedPath))
                {
                    throw new InvalidOperationException(
                        "The loaded project assembly has no file location."
                    );
                }

                // 5) Report the assembly name and actual path
                Log($"Project assembly loaded: {projectAssembly.GetName().Name}");
                Log($"Project assembly loaded path: {loadedPath}");

                // 6) Transfer the output string ownership to C++
                *outLoadedAssemblyPath = Marshaling.ConvertStringToNative(loadedPath);

                return 0;
            }
            catch (Exception exception)
            {
                // 7) Report failure without propagating logging exceptions
                try
                {
                    Log($"Failed to load project assembly: {path}\n{exception}");
                }
                catch
                {
                }

                return -5;
            }
        }

        [UnmanagedCallersOnly]
        private static unsafe int EnsureProjectSolution(char* projectDirectory, char* sdkDirectory)
        {
            try
            {
                // 1) Validate the native pointers
                if (projectDirectory == null || sdkDirectory == null)
                {
                    return -1;
                }

                // 2) Copy the native strings into managed strings
                string projectPath = new string(projectDirectory);
                string sdkPath = new string(sdkDirectory);

                // 3) Generate the missing project files
                MingTools.ProjectGenerator.EnsureProjectSolution(projectPath, sdkPath);

                return 0;
            }
            catch (Exception exception)
            {
                // 4) Report the error without crossing the native boundary
                try
                {
                    Log($"Failed to ensure C# project files:\n{exception}");
                }
                catch
                {
                }

                return -5;
            }
        }
    }
}
