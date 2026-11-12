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
    public delegate* unmanaged<IntPtr, String*, ming_callable*, void> CreateCallable;
    public delegate* unmanaged<ming_callable*, ming_callable*, void> CopyCallable;
    public delegate* unmanaged<ming_callable*, void> DestroyCallable;
    public delegate* unmanaged<IntPtr, String*, ming_callable*, int> ConnectSignal;
    public delegate* unmanaged<IntPtr, String*, ming_callable*, int> IsSignalConnected;
    public delegate* unmanaged<IntPtr, String*, ming_callable*, int> DisconnectSignal;
    public delegate* unmanaged<ming_variant*, ming_variant*, void> CopyVariant;
    public delegate* unmanaged<IntPtr, String*, ming_variant**, int, int> EmitSignal;
    public delegate* unmanaged<IntPtr, IntPtr, IntPtr, ming_callable*, int> CreateManagedCallable;
    public delegate* unmanaged<ming_callable*, ming_variant**, int, ming_variant*, ming_call_error*, void> CallCallable;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ManagedCallbacks
{
    public delegate* unmanaged<IntPtr, String*, int> AddScriptBridge;
    public delegate* unmanaged<IntPtr, int> RemoveScriptBridge;
    public delegate* unmanaged<String*, IntPtr, IntPtr> CreateNativeManagedInstance;
    public delegate* unmanaged<IntPtr, IntPtr, IntPtr> CreateUserManagedInstance;
    public delegate* unmanaged<IntPtr, void> ReleaseGCHandle;
    public delegate* unmanaged<IntPtr, String*, ming_variant**, int, ming_variant*, ming_call_error*, void> Call;
    public delegate* unmanaged<IntPtr, String*, int> HasMethod;
    public delegate* unmanaged<String*, void> GetScriptBaseTypes;
    public delegate* unmanaged<String*, String*, int> GetScriptNativeBase;
    public delegate* unmanaged<void> DrainResourceBindings;
    public delegate* unmanaged<void> DisposeResourceBindings;
    public delegate* unmanaged<IntPtr, int, IntPtr> UpdateNativeBindingGCHandle;
    public delegate* unmanaged<IntPtr, IntPtr, ming_variant**, int, ming_variant*, ming_call_error*, void> InvokeDelegate;
    public delegate* unmanaged<IntPtr, IntPtr, int> DelegateEquals;
    public delegate* unmanaged<IntPtr, void> ReleaseDelegateGCHandle;
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
            || sizeof(ming_call_error) != 16
            || Marshal.OffsetOf<ming_call_error>(nameof(ming_call_error.Error)).ToInt32() != 0
            || Marshal.OffsetOf<ming_call_error>(nameof(ming_call_error.Argument)).ToInt32() != 4
            || Marshal.OffsetOf<ming_call_error>(nameof(ming_call_error.ExpectedArgumentCount)).ToInt32() != 8
            || Marshal.OffsetOf<ming_call_error>(nameof(ming_call_error.ExpectedType)).ToInt32() != 12
            || sizeof(ming_callable) != 24
            || sizeof(ming_variant) != 24
            || sizeof(ming_variant.movable) != 24
            || Marshal.OffsetOf<ming_callable>(nameof(ming_callable.ObjectID)).ToInt32() != 0
            || Marshal.OffsetOf<ming_callable>(nameof(ming_callable.Method)).ToInt32() != 8
            || Marshal.OffsetOf<ming_callable>(nameof(ming_callable.Managed)).ToInt32() != 16
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
            HasMethod = &ScriptManagerBridge.HasMethod,
            GetScriptBaseTypes = &ScriptManagerBridge.GetScriptBaseTypes,
            GetScriptNativeBase = &ScriptManagerBridge.GetScriptNativeBase,
            DrainResourceBindings = &ScriptManagerBridge.DrainResourceBindings,
            DisposeResourceBindings = &ScriptManagerBridge.DisposeResourceBindings,
            UpdateNativeBindingGCHandle = &ScriptManagerBridge.UpdateNativeBindingGCHandle,
            InvokeDelegate = &ScriptManagerBridge.InvokeDelegate,
            DelegateEquals = &ScriptManagerBridge.DelegateEquals,
            ReleaseDelegateGCHandle = &ScriptManagerBridge.ReleaseDelegateGCHandle
        };
    }

    #endregion

    #region Callable and Signals

    internal static ming_callable CreateManagedCallable(IntPtr target, IntPtr handle, IntPtr trampoline)
    {
        ming_callable result = default;
        if (s_callbacks.CreateManagedCallable(target, handle, trampoline, &result) == 0)
        {
            throw new InvalidOperationException("Managed Callable creation is unavailable while scripts are unloading or shut down.");
        }

        return result;
    }

    internal static Variant CallCallable(Callable callback, Variant[] arguments, out ming_call_error error)
    {
        ArgumentNullException.ThrowIfNull(arguments);
        ScriptManagerBridge.EnsureNativeBindings();
        ming_callable nativeCallback = callback.BorrowNativeValue();
        var values = new ming_variant.movable[arguments.Length];
        for (int i = 0; i < arguments.Length; i++)
        {
            values[i] = (ming_variant.movable)arguments[i].BorrowNativeValue();
        }

        ming_variant result = default;
        ming_call_error callError = new();
        try
        {
            fixed (ming_variant.movable* valuesPtr = values)
            {
                var pointers = new IntPtr[arguments.Length];
                for (int i = 0; i < arguments.Length; i++)
                {
                    pointers[i] = (IntPtr)(valuesPtr + i);
                }

                fixed (IntPtr* args = pointers)
                {
                    s_callbacks.CallCallable(&nativeCallback, (ming_variant**)args, arguments.Length, &result, &callError);
                }
            }

            error = callError;
            return Variant.TakeOwnershipOfNative(ref result);
        }
        finally
        {
            GC.KeepAlive(callback);
            GC.KeepAlive(arguments);
        }
    }

    internal static ming_callable CreateCallable(IntPtr target, string methodName)
    {
        using String name = Marshaling.ConvertStringToNative(methodName);
        ming_callable result = default;
        s_callbacks.CreateCallable(target, &name, &result);
        return result;
    }

    internal static ming_callable CopyCallable(in ming_callable source)
    {
        ming_callable result = default;
        fixed (ming_callable* sourcePtr = &source)
        {
            s_callbacks.CopyCallable(sourcePtr, &result);
        }

        return result;
    }

    internal static void DestroyCallable(ref ming_callable value)
    {
        fixed (ming_callable* valuePtr = &value)
        {
            s_callbacks.DestroyCallable(valuePtr);
        }
    }

    internal static bool ConnectSignal(MingObject source, string name, Callable callback)
    {
        return InvokeSignalOperation(s_callbacks.ConnectSignal, source, name, callback);
    }

    internal static bool IsSignalConnected(MingObject source, string name, Callable callback)
    {
        return InvokeSignalOperation(s_callbacks.IsSignalConnected, source, name, callback);
    }

    internal static bool DisconnectSignal(MingObject source, string name, Callable callback)
    {
        return InvokeSignalOperation(s_callbacks.DisconnectSignal, source, name, callback);
    }

    internal static bool EmitSignal(MingObject source, string name, Variant[] arguments)
    {
        ArgumentNullException.ThrowIfNull(name);
        ArgumentNullException.ThrowIfNull(arguments);
        ScriptManagerBridge.EnsureNativeBindings();
        IntPtr sourcePtr = MingObject.GetPtr(source);
        var values = new ming_variant.movable[arguments.Length];
        for (int i = 0; i < arguments.Length; i++)
        {
            ming_variant value = arguments[i].BorrowNativeValue();
            values[i] = (ming_variant.movable)value;
        }

        using String nativeName = Marshaling.ConvertStringToNative(name);
        try
        {
            fixed (ming_variant.movable* valuesPtr = values)
            {
                var pointers = new IntPtr[arguments.Length];
                for (int i = 0; i < arguments.Length; i++)
                {
                    pointers[i] = (IntPtr)(valuesPtr + i);
                }

                fixed (IntPtr* args = pointers)
                {
                    return s_callbacks.EmitSignal(sourcePtr, &nativeName, (ming_variant**)args, arguments.Length) != 0;
                }
            }
        }
        finally
        {
            GC.KeepAlive(arguments);
            GC.KeepAlive(source);
        }
    }

    private static bool InvokeSignalOperation(
        delegate* unmanaged<IntPtr, String*, ming_callable*, int> operation,
        MingObject source, string name, Callable callback)
    {
        ArgumentNullException.ThrowIfNull(name);
        ArgumentNullException.ThrowIfNull(callback);

        IntPtr sourcePtr = MingObject.GetPtr(source);
        ming_callable nativeCallback = callback.BorrowNativeValue();
        using String nativeName = Marshaling.ConvertStringToNative(name);
        bool result = operation(sourcePtr, &nativeName, &nativeCallback) != 0;
        GC.KeepAlive(callback);
        GC.KeepAlive(source);
        return result;
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

    internal static ming_variant CopyVariant(in ming_variant source)
    {
        ScriptManagerBridge.EnsureNativeBindings();
        ming_variant result = default;
        fixed (ming_variant* value = &source)
        {
            s_callbacks.CopyVariant(value, &result);
        }

        return result;
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
