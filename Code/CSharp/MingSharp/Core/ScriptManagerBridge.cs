namespace Ming;

using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;
using System.Text.Json;

public static class ScriptManagerBridge
{
    private static readonly Dictionary<IntPtr, Type> s_scriptTypes = new();
    private static readonly Dictionary<string, Type> s_scriptPathTypes = new(StringComparer.Ordinal);
    private static readonly Dictionary<AssemblyLoadContext, Dictionary<IntPtr, object>> s_strongReferencesByAlc = new();
    private static readonly ConditionalWeakTable<AssemblyLoadContext, object> s_unloadingAlcs = new();
    private static readonly Dictionary<AssemblyLoadContext, HashSet<Type>> s_typesByAlc = new();

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

        if (s_scriptTypes.TryGetValue(scriptPtr, out Type existing))
        {
            if (existing != scriptType)
            {
                throw new InvalidOperationException(
                    "The script resource is already associated with another type.");
            }

            return;
        }

        TrackScriptType(scriptType);
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

        TrackScriptType(scriptType);
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

            Type nativeBase = scriptType;
            while (nativeBase != null && nativeBase.Assembly != typeof(MingObject).Assembly)
            {
                nativeBase = nativeBase.BaseType;
            }

            string ownerClass = NativeFuncs.GetClassName(ownerPtr);
            Type ownerType = ownerClass == "Object" ? typeof(MingObject)
                : typeof(MingObject).Assembly.GetType("Ming." + ownerClass);
            if (nativeBase == null || ownerType == null || !nativeBase.IsAssignableFrom(ownerType))
            {
                throw new InvalidOperationException($"Script '{scriptType}' cannot attach to native '{ownerClass}'.");
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

            GCHandle gcHandle = AllocScriptGCHandle(instance);
            try
            {
                NativeFuncs.TrackScriptInstanceAllocated(GCHandle.ToIntPtr(gcHandle), instance);
                return GCHandle.ToIntPtr(gcHandle);
            }
            catch
            {
                instance.Dispose();
                FreeScriptGCHandle(gcHandle);
                throw;
            }
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
            s_scriptTypes.Remove(scriptPtr);
            return 1;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return 0;
        }
    }


    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void OnHandlesAlcUnloading(AssemblyLoadContext alc)
    {
        // 1) Prevent new strong references during unloading
        s_unloadingAlcs.GetValue(alc, static _ => new object());

        // 2) Release this context's strong references
        if (s_strongReferencesByAlc.Remove(alc, out var references))
        {
            references.Clear();
        }
    }

    [UnmanagedCallersOnly]
    internal static unsafe int SerializeState(
    IntPtr handlePtr,
    MingString* outState)
    {
        if (outState == null)
        {
            return 0;
        }

        *outState = default;

        try
        {
            if (handlePtr == IntPtr.Zero ||
                GCHandle.FromIntPtr(handlePtr).Target is not MingObject instance)
            {
                return 0;
            }

            var state = new Dictionary<string, string>(StringComparer.Ordinal);
            instance.SaveReloadState(state);

            string json = JsonSerializer.Serialize(state);
            *outState = Marshaling.ConvertStringToNative(json);
            return 1;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    internal static int DeserializeState(
        IntPtr handlePtr,
        IntPtr statePtr,
        int stateLength)
    {
        try
        {
            if (handlePtr == IntPtr.Zero ||
                statePtr == IntPtr.Zero ||
                stateLength <= 0 ||
                GCHandle.FromIntPtr(handlePtr).Target is not MingObject instance)
            {
                return 0;
            }

            string json = Marshal.PtrToStringUTF8(statePtr, stateLength);
            var state =
                JsonSerializer.Deserialize<Dictionary<string, string>>(json);

            if (state == null)
            {
                return 0;
            }

            instance.RestoreReloadState(state);
            return 1;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return 0;
        }
    }

    internal static GCHandle AllocScriptGCHandle(MingObject instance)
    {
        AssemblyLoadContext alc =
            AssemblyLoadContext.GetLoadContext(instance.GetType().Assembly);

        if (alc == null || !alc.IsCollectible)
        {
            return GCHandle.Alloc(instance, GCHandleType.Normal);
        }

        if (s_unloadingAlcs.TryGetValue(alc, out _))
        {
            throw new InvalidOperationException(
                "Cannot create a script while its assembly is unloading.");
        }

        if (!s_strongReferencesByAlc.TryGetValue(alc, out var references))
        {
            references = new Dictionary<IntPtr, object>();
            s_strongReferencesByAlc.Add(alc, references);
            alc.Unloading += OnHandlesAlcUnloading;
        }

        GCHandle handle = GCHandle.Alloc(instance, GCHandleType.Weak);

        try
        {
            references.Add(GCHandle.ToIntPtr(handle), instance);
            return handle;
        }
        catch
        {
            handle.Free();
            throw;
        }
    }

    internal static void FreeScriptGCHandle(GCHandle handle)
    {
        object target = handle.Target;

        if (target != null)
        {
            AssemblyLoadContext alc =
                AssemblyLoadContext.GetLoadContext(target.GetType().Assembly);

            if (alc != null &&
                s_strongReferencesByAlc.TryGetValue(alc, out var references))
            {
                references.Remove(GCHandle.ToIntPtr(handle));
            }
        }

        handle.Free();
    }

    private static void TrackScriptType(Type type)
    {
        AssemblyLoadContext alc = AssemblyLoadContext.GetLoadContext(type.Assembly);

        if (alc == null || !alc.IsCollectible)
        {
            return;
        }

        if (s_unloadingAlcs.TryGetValue(alc, out _))
        {
            throw new InvalidOperationException(
                "Cannot register a type from an unloading assembly.");
        }

        if (!s_typesByAlc.TryGetValue(alc, out var types))
        {
            types = new HashSet<Type>();
            s_typesByAlc.Add(alc, types);
            alc.Unloading += OnTypesAlcUnloading;
        }

        types.Add(type);
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void OnTypesAlcUnloading(AssemblyLoadContext alc)
    {
        // 1) Mark the context before removing its registrations
        s_unloadingAlcs.GetValue(alc, static _ => new object());

        if (!s_typesByAlc.Remove(alc, out var types))
        {
            return;
        }

        // 2) Remove native resource associations
        foreach (var entry in s_scriptTypes.ToArray())
        {
            if (types.Contains(entry.Value))
            {
                s_scriptTypes.Remove(entry.Key);
            }
        }

        // 3) Remove script path associations
        foreach (var entry in s_scriptPathTypes.ToArray())
        {
            if (types.Contains(entry.Value))
            {
                s_scriptPathTypes.Remove(entry.Key);
            }
        }

        types.Clear();
    }

    public static void LookupScriptsInAssembly(Assembly assembly)
    {
        foreach (Type type in assembly.GetTypes())
        {
            if (type.IsAbstract ||
                type.ContainsGenericParameters ||
                !typeof(MingObject).IsAssignableFrom(type))
            {
                continue;
            }

            ScriptPathAttribute attribute =
                type.GetCustomAttribute<ScriptPathAttribute>(inherit: false);

            if (attribute == null)
            {
                continue;
            }

            AddScriptType(attribute.Path, type);
        }
    }

    #endregion
}
