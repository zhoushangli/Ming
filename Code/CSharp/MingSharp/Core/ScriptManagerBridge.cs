namespace Ming;

using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

public static class ScriptManagerBridge
{
    private static readonly Dictionary<IntPtr, Type> s_scriptTypes = new();
    private static readonly Dictionary<string, Type> s_scriptPathTypes = new(StringComparer.Ordinal);
    private static readonly Dictionary<AssemblyLoadContext, Dictionary<IntPtr, object>> s_strongReferencesByAlc = new();
    private static readonly ConditionalWeakTable<AssemblyLoadContext, object> s_unloadingAlcs = new();
    private static readonly Dictionary<AssemblyLoadContext, HashSet<Type>> s_typesByAlc = new();

    // Classify live script handles independently of diagnostic counters.
    // e.g. ReleaseGCHandleCore uses membership to remove collectible-context references.
    private static readonly HashSet<IntPtr> s_scriptInstanceHandles = new();
    private static int s_scriptInstanceHandleAllocated;
    private static int s_scriptInstanceHandleFreed;
    private static int s_nativeBindingHandleAllocated;
    private static int s_nativeBindingHandleFreed;

    #region Type Registration

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

    internal static bool TryGetScriptType(IntPtr scriptPtr, out Type scriptType)
    {
        return s_scriptTypes.TryGetValue(scriptPtr, out scriptType);
    }

    public static bool TryGetScriptType(string scriptPath, out Type scriptType)
    {
        return s_scriptPathTypes.TryGetValue(scriptPath, out scriptType);
    }

    internal static void RemoveScriptType(IntPtr scriptPtr)
    {
        s_scriptTypes.Remove(scriptPtr);
    }

    [UnmanagedCallersOnly]
    internal static unsafe int AddScriptBridge(
        IntPtr scriptPtr,
        String* scriptPathPtr)
    {
        if (
            scriptPtr == IntPtr.Zero
            || scriptPathPtr == null
        )
        {
            return 0;
        }

        try
        {
            // 1) Convert the native UTF-32 path
            string scriptPath = Marshaling.ConvertStringToManaged(*scriptPathPtr);

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

    #region Instance Functions

    [UnmanagedCallersOnly]
    internal static unsafe IntPtr CreateNativeManagedInstance(String* nativeClassNamePtr, IntPtr ownerPtr)
    {
        if (nativeClassNamePtr == null || ownerPtr == IntPtr.Zero)
        {
            return IntPtr.Zero;
        }

        try
        {
            string className = Marshaling.ConvertStringToManaged(*nativeClassNamePtr);
            MingObject wrapper = Constructors.Invoke(className, ownerPtr);
            return AllocNativeBindingGCHandle(wrapper);
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return IntPtr.Zero;
        }
    }

    [UnmanagedCallersOnly]
    internal static unsafe IntPtr CreateUserManagedInstance(IntPtr scriptPtr, IntPtr ownerPtr)
    {
        if (scriptPtr == IntPtr.Zero || ownerPtr == IntPtr.Zero)
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

            return GCHandle.ToIntPtr(AllocScriptGCHandle(instance));
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return IntPtr.Zero;
        }
    }

    [UnmanagedCallersOnly]
    internal static unsafe int Call(
        IntPtr objectGCHandle,
        String* methodName,
        ming_variant** args,
        int argc,
        ming_variant* ret)
    {
        if (objectGCHandle == IntPtr.Zero || methodName == null)
        {
            return 0;
        }

        try
        {
            var mingObject = (MingObject)GCHandle.FromIntPtr(objectGCHandle).Target;

            if (mingObject == null)
            {
                throw new InvalidOperationException(
                    "The managed object has been garbage collected."
                );
            }

            ming_variant retVariant = default;
            bool methodInvoke = mingObject.InvokeMingClassMethod(
                in *methodName,
                new NativeVariantPtrArgs(args, argc),
                out retVariant
            );

            if (!methodInvoke)
            {
                *ret = default;

                throw new MissingMethodException(
                    mingObject.GetType().FullName,
                    methodName->ToString()
                );
            }

            *ret = retVariant;
            return 1;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return 0;
        }
    }

    #endregion

    #region Handle Management

    internal static IntPtr AllocNativeBindingGCHandle(MingObject instance)
    {
        GCHandle handle = GCHandle.Alloc(instance, GCHandleType.Normal);
        s_nativeBindingHandleAllocated++;
        return GCHandle.ToIntPtr(handle);
    }

    private static GCHandle AllocScriptGCHandle(MingObject instance)
    {
        GCHandle handle = AllocScriptGCHandleCore(instance);
        try
        {
            s_scriptInstanceHandles.Add(GCHandle.ToIntPtr(handle));
            s_scriptInstanceHandleAllocated++;
            return handle;
        }
        catch
        {
            instance.Dispose();
            FreeScriptGCHandle(handle);
            throw;
        }
    }

    private static GCHandle AllocScriptGCHandleCore(MingObject instance)
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

    [UnmanagedCallersOnly]
    internal static void ReleaseGCHandle(IntPtr handlePtr)
    {
        ReleaseGCHandleCore(handlePtr);
    }

    // Release a handle after normal teardown or failed ownership transfer.
    // e.g. ReleaseGCHandleCore(handlePtr) invalidates the wrapper before freeing its handle.
    internal static void ReleaseGCHandleCore(IntPtr handlePtr)
    {
        if (handlePtr == IntPtr.Zero)
            return;

        try
        {
            GCHandle handle = GCHandle.FromIntPtr(handlePtr);
            bool isScriptInstance = s_scriptInstanceHandles.Contains(handlePtr);
            try
            {
                if (handle.Target is MingObject instance)
                    instance.Dispose();
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine(exception);
            }

            if (isScriptInstance)
            {
                FreeScriptGCHandle(handle);
                s_scriptInstanceHandles.Remove(handlePtr);
                s_scriptInstanceHandleFreed++;
            }
            else
            {
                handle.Free();
                s_nativeBindingHandleFreed++;
            }
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
        }
    }

    private static void FreeScriptGCHandle(GCHandle handle)
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

    #endregion

    #region Assembly Context Cleanup

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

    #endregion

    #region Diagnostics

    public static string GetBindingSummary()
    {
        int allocated = s_nativeBindingHandleAllocated + s_scriptInstanceHandleAllocated;
        int freed = s_nativeBindingHandleFreed + s_scriptInstanceHandleFreed;
        string severity = allocated != freed ? "[Warning] " : string.Empty;
        return severity + "Managed binding summary (active = allocated - freed):\n"
            + $"  NativeBinding: allocated={s_nativeBindingHandleAllocated}, freed={s_nativeBindingHandleFreed}, active={s_nativeBindingHandleAllocated - s_nativeBindingHandleFreed}\n"
            + $"  ScriptInstance: allocated={s_scriptInstanceHandleAllocated}, freed={s_scriptInstanceHandleFreed}, active={s_scriptInstanceHandleAllocated - s_scriptInstanceHandleFreed}\n"
            + $"  Total: allocated={allocated}, freed={freed}, active={allocated - freed}";
    }

    #endregion
}
