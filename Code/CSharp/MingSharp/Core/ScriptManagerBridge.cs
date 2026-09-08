namespace Ming;

using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;


public static class ScriptManagerBridge
{
    private static readonly Dictionary<IntPtr, Type> s_scriptTypes = new();
    private static readonly Dictionary<string, Type> s_scriptPathTypes = new(StringComparer.Ordinal);

    internal static void AddScriptType(IntPtr scriptPtr, Type scriptType)
    {
        if (scriptPtr == IntPtr.Zero)
        {
            throw new ArgumentException(
                "Script pointer cannot be zero.",
                nameof(scriptPtr)
            );
        }

        if (
            scriptType.IsAbstract
            || !typeof(MingObject).IsAssignableFrom(scriptType)
        )
        {
            throw new ArgumentException(
                $"Type '{scriptType}' is not an instantiable MingObject script."
            );
        }

        s_scriptTypes.Add(scriptPtr, scriptType);
    }

    internal static bool TryGetScriptType(IntPtr scriptPtr, out Type scriptType)
    {
        return s_scriptTypes.TryGetValue(scriptPtr, out scriptType);
    }

    public static void AddScriptType(string scriptPath, Type scriptType)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(scriptPath);
        ArgumentNullException.ThrowIfNull(scriptType);

        if (
            scriptType.IsAbstract
            || scriptType.ContainsGenericParameters
            || !typeof(MingObject).IsAssignableFrom(scriptType)
        )
        {
            throw new ArgumentException(
                $"Type '{scriptType}' is not an instantiable MingObject script.",
                nameof(scriptType)
            );
        }

        s_scriptPathTypes.Add(scriptPath, scriptType);
    }

    public static bool TryGetScriptType(string scriptPath, out Type scriptType)
    {
        return s_scriptPathTypes.TryGetValue(scriptPath, out scriptType);
    }

    internal static void RemoveScriptType(IntPtr scriptPtr)
    {
        s_scriptTypes.Remove(scriptPtr);
    }

    #region Managed Callbacks

    [UnmanagedCallersOnly]
    internal static int AddScriptBridge(
        IntPtr scriptPtr,
        IntPtr scriptPathPtr,
        int scriptPathLength)
    {
        if (
            scriptPtr == IntPtr.Zero
            || scriptPathPtr == IntPtr.Zero
            || scriptPathLength <= 0
        )
        {
            return 0;
        }

        try
        {
            // 1) Copy the native UTF-8 path
            string scriptPath = Marshal.PtrToStringUTF8(
                scriptPathPtr,
                scriptPathLength
            );

            // 2) Resolve the registered script type
            if (!TryGetScriptType(scriptPath, out Type scriptType))
            {
                throw new InvalidOperationException(
                    $"No managed type is registered for '{scriptPath}'."
                );
            }

            // 3) Associate the native script resource with the type
            AddScriptType(scriptPtr, scriptType);
            return 1;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    internal unsafe static IntPtr CreateUserManagedInstance(IntPtr scriptPtr, IntPtr ownerPtr)
    {
        if (scriptPtr == IntPtr.Zero || ownerPtr == IntPtr.Zero
        )
        {
            return IntPtr.Zero;
        }

        try
        {
            if (!ScriptManagerBridge.TryGetScriptType(scriptPtr, out Type scriptType) || scriptType == null)
            {
                throw new InvalidOperationException(
                    "No managed type is registered for the CSharpScript."
                );
            }

            ConstructorInfo constructor = scriptType
                .GetConstructors(
                    BindingFlags.Public
                    | BindingFlags.NonPublic
                    | BindingFlags.Instance
                )
                .FirstOrDefault(
                    candidate =>
                        candidate.GetParameters().Length == 0
                );

            if (constructor == null)
            {
                throw new MissingMethodException(
                    scriptType.FullName,
                    ".ctor()"
                );
            }

            var instance = (MingObject)RuntimeHelpers.GetUninitializedObject(scriptType);

            instance.NativePtr = ownerPtr;

            _ = constructor.Invoke(instance, Array.Empty<object>());

            GCHandle gcHandle = GCHandle.Alloc(instance, GCHandleType.Normal);
            NativeFuncs.TrackScriptInstanceAllocated(GCHandle.ToIntPtr(gcHandle), instance);
            return GCHandle.ToIntPtr(gcHandle);
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return IntPtr.Zero;
        }
    }

    [UnmanagedCallersOnly]
    internal unsafe static IntPtr CreateNativeManagedInstance(IntPtr nativeClassNamePtr, IntPtr ownerPtr)
    {
        if (nativeClassNamePtr == IntPtr.Zero || ownerPtr == IntPtr.Zero)
        {
            return IntPtr.Zero;
        }

        try
        {
            MingString* nativeClassName = (MingString*)nativeClassNamePtr;
            string className = Marshaling.ConvertStringToManaged(*nativeClassName);
            MingObject wrapper = Constructors.Invoke(className, ownerPtr);
            GCHandle gcHandle = GCHandle.Alloc(wrapper, GCHandleType.Normal);
            NativeFuncs.TrackNativeBindingAllocated();
            return GCHandle.ToIntPtr(gcHandle);
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return IntPtr.Zero;
        }
    }

    [UnmanagedCallersOnly]
    internal static int RemoveScriptBridge(IntPtr scriptPtr)
    {
        if (scriptPtr == IntPtr.Zero)
        {
            return 0;
        }

        try
        {
            return s_scriptTypes.Remove(scriptPtr) ? 1 : 0;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return 0;
        }
    }

    #endregion
}
