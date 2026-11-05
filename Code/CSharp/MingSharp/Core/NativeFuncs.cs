namespace Ming;

using System.Runtime.InteropServices;


[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeCallbacks
{
    public delegate* unmanaged<String*, int> Log;
    public delegate* unmanaged<uint*, int, String*, void> CreateString;
    public delegate* unmanaged<String*, void> DestroyString;
    public delegate* unmanaged<String*, String*, IntPtr> GetMethodBind;
    public delegate* unmanaged<IntPtr, IntPtr, void**, void*, void> MethodBindPtrCall;
    public delegate* unmanaged<String*, delegate* unmanaged<nint>> GetConstructor;
    public delegate* unmanaged<IntPtr, String*, void> GetClassName;
    public delegate* unmanaged<IntPtr, IntPtr, int> TieNativeManagedToUnmanaged;
    public delegate* unmanaged<IntPtr, IntPtr> UnmanagedGetInstanceBindingManaged;
    public delegate* unmanaged<IntPtr, IntPtr> UnmanagedInstanceBindingCreateManaged;
    public delegate* unmanaged<IntPtr, int> RetainRef;
    public delegate* unmanaged<IntPtr, void> ReleaseRef;
    public delegate* unmanaged<IntPtr, IntPtr, void> DetachBinding;
    public delegate* unmanaged<IntPtr, ming_variant*, void> CreateObjectVariant;
    public delegate* unmanaged<ming_variant*, void> DestroyVariant;
    public delegate* unmanaged<String*, IntPtr> GetGlobalObject;
    public delegate* unmanaged<IntPtr, int*, IntPtr> GetScriptInstanceManaged;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ManagedCallbacks
{
    public delegate* unmanaged<IntPtr, String*, int> AddScriptBridge;
    public delegate* unmanaged<IntPtr, int> RemoveScriptBridge;
    public delegate* unmanaged<String*, IntPtr, IntPtr> CreateNativeManagedInstance;
    public delegate* unmanaged<IntPtr, IntPtr, IntPtr> CreateUserManagedInstance;
    public delegate* unmanaged<IntPtr, void> ReleaseGCHandle;
    public delegate* unmanaged<IntPtr, String*, ming_variant**, int, ming_variant*, int> Call;
    public delegate* unmanaged<String*, void> GetScriptBaseTypes;
    public delegate* unmanaged<String*, String*, int> GetScriptNativeBase;
    public delegate* unmanaged<void> DrainResourceBindings;
    public delegate* unmanaged<void> DisposeResourceBindings;
    public delegate* unmanaged<IntPtr, int, IntPtr> UpdateNativeBindingGCHandle;
}

public static unsafe class NativeFuncs
{
    private static NativeCallbacks s_callbacks;

    internal static IntPtr GetScriptInstanceManaged(IntPtr native, out bool hasScript)
    {
        int found = 0;
        IntPtr handle = s_callbacks.GetScriptInstanceManaged(native, &found);
        hasScript = found != 0;
        return handle;
    }

    internal static IntPtr GetGlobalObject(string name)
    {
        using String nativeName = Marshaling.ConvertStringToNative(name);
        return s_callbacks.GetGlobalObject(&nativeName);
    }

    #region Initialization

    // Store the native function pointer table passed from the engine.
    // e.g. NativeFuncs.Initialize(callbacksPtr, sizeof(NativeCallbacks))
    public static void Initialize(
        IntPtr nativeCallbacks,
        int nativeCallbackSize,
        IntPtr managedCallbacks,
        int managedCallbackSize
    )
    {
        if (
            nativeCallbackSize != sizeof(NativeCallbacks)
            || managedCallbackSize != sizeof(ManagedCallbacks)
        )
        {
            throw new InvalidOperationException("Callbacks size mismatch.");
        }

        s_callbacks = *(NativeCallbacks*)nativeCallbacks;
        ScriptManagerBridge.StartNativeBindings();
        *(ManagedCallbacks*)managedCallbacks = new ManagedCallbacks
        {
            AddScriptBridge = &ScriptManagerBridge.AddScriptBridge,
            RemoveScriptBridge = &ScriptManagerBridge.RemoveScriptBridge,
            CreateNativeManagedInstance = &ScriptManagerBridge.CreateNativeManagedInstance,
            CreateUserManagedInstance = &ScriptManagerBridge.CreateUserManagedInstance,
            ReleaseGCHandle = &ScriptManagerBridge.ReleaseGCHandle,
            Call = &ScriptManagerBridge.Call,
            GetScriptBaseTypes = &ScriptManagerBridge.GetScriptBaseTypes,
            GetScriptNativeBase = &ScriptManagerBridge.GetScriptNativeBase,
            DrainResourceBindings = &ScriptManagerBridge.DrainResourceBindings,
            DisposeResourceBindings = &ScriptManagerBridge.DisposeResourceBindings,
            UpdateNativeBindingGCHandle = &ScriptManagerBridge.UpdateNativeBindingGCHandle
        };
    }

    #endregion

    #region Logging

    public static int Log(string message)
    {
        using String text = Marshaling.ConvertStringToNative(message);
        return s_callbacks.Log(&text);
    }

    #endregion

    #region Strings

    // Copy UTF-32 code points into the string owned by the managed caller.
    // e.g. CreateString(codes, 4, &result) leaves result owning 4 code points.
    internal static void CreateString(uint* codePoints, int length, String* outString)
    {
        s_callbacks.CreateString(codePoints, length, outString);
    }

    internal static void DestroyString(String* str)
    {
        s_callbacks.DestroyString(str);
    }

    #endregion

    #region Method Binding

    internal static IntPtr GetMethodBind(string className, string methodName)
    {
        using String nativeClass = Marshaling.ConvertStringToNative(className);
        using String nativeMethod = Marshaling.ConvertStringToNative(methodName);
        IntPtr binding = s_callbacks.GetMethodBind(&nativeClass, &nativeMethod);
        if (binding == IntPtr.Zero)
        {
            throw new MissingMethodException($"Native method is not registered: {className}.{methodName}");
        }
        return binding;
    }

    internal static void MethodBindPtrCall(
        IntPtr methodBind,
        IntPtr objPtr,
        void** args,
        void* retPtr
    )
    {
        s_callbacks.MethodBindPtrCall(methodBind, objPtr, args, retPtr);
    }

    internal static delegate* unmanaged<IntPtr> GetConstructor(in String name)
    {
        fixed (String* namePtr = &name)
        {
            return s_callbacks.GetConstructor(namePtr);
        }
    }

    #endregion

    #region Object Binding

    internal static string GetClassName(IntPtr objPtr)
    {
        using String name = default;
        s_callbacks.GetClassName(objPtr, &name);
        return Marshaling.ConvertStringToManaged(name);
    }

    internal static bool TieNativeManagedToUnmanaged(IntPtr gcHandle, IntPtr native)
    {
        if (gcHandle == IntPtr.Zero || native == IntPtr.Zero)
        {
            return false;
        }

        return s_callbacks.TieNativeManagedToUnmanaged(gcHandle, native) != 0;
    }

    internal static IntPtr UnmanagedGetInstanceBindingManaged(IntPtr native)
    {
        return native != IntPtr.Zero
            ? s_callbacks.UnmanagedGetInstanceBindingManaged(native)
            : IntPtr.Zero;
    }

    internal static IntPtr UnmanagedInstanceBindingCreateManaged(IntPtr native)
    {
        return native != IntPtr.Zero
            ? s_callbacks.UnmanagedInstanceBindingCreateManaged(native)
            : IntPtr.Zero;
    }

    #endregion
    internal static void DestroyVariant(ref ming_variant variant)
    {
        fixed (ming_variant* value = &variant)
        {
            s_callbacks.DestroyVariant(value);
        }
    }

    internal static bool RetainRef(IntPtr ptr) => s_callbacks.RetainRef(ptr) != 0;
    internal static void ReleaseRef(IntPtr ptr) => s_callbacks.ReleaseRef(ptr);
    internal static void DetachBinding(IntPtr ptr, IntPtr handle) => s_callbacks.DetachBinding(ptr, handle);
    internal static ming_variant CreateObjectVariant(IntPtr ptr)
    {
        ming_variant result = default;
        s_callbacks.CreateObjectVariant(ptr, &result);
        return result;
    }
}
