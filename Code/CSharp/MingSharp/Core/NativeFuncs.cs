namespace Ming;

using System.Runtime.InteropServices;


[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeCallbacks
{
    public delegate* unmanaged<MingString*, int> Log;
    public delegate* unmanaged<byte*, int, IntPtr> CreateString;
    public delegate* unmanaged<IntPtr, byte*> GetStringBuffer;
    public delegate* unmanaged<IntPtr, int> GetStringLength;
    public delegate* unmanaged<IntPtr, void> DestroyString;
    public delegate* unmanaged<MingString*, MingString*, IntPtr> GetMethodBind;
    public delegate* unmanaged<IntPtr, IntPtr, void**, void*, void> MethodBindPtrCall;
    public delegate* unmanaged<MingString*, delegate* unmanaged<nint>> GetConstructor;
    public delegate* unmanaged<IntPtr, MingString*, void> GetClassName;
    public delegate* unmanaged<IntPtr, IntPtr, int> TieNativeManagedToUnmanaged;
    public delegate* unmanaged<IntPtr, IntPtr> UnmanagedGetInstanceBindingManaged;
    public delegate* unmanaged<IntPtr, IntPtr> UnmanagedInstanceBindingCreateManaged;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ManagedCallbacks
{
    public delegate* unmanaged<IntPtr, MingString*, int> AddScriptBridge;
    public delegate* unmanaged<IntPtr, int> RemoveScriptBridge;
    public delegate* unmanaged<MingString*, IntPtr, IntPtr> CreateNativeManagedInstance;
    public delegate* unmanaged<IntPtr, IntPtr, IntPtr> CreateUserManagedInstance;
    public delegate* unmanaged<IntPtr, void> ReleaseGCHandle;
}

public static unsafe class NativeFuncs
{
    private static NativeCallbacks s_callbacks;

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
        *(ManagedCallbacks*)managedCallbacks = new ManagedCallbacks
        {
            AddScriptBridge = &ScriptManagerBridge.AddScriptBridge,
            RemoveScriptBridge = &ScriptManagerBridge.RemoveScriptBridge,
            CreateNativeManagedInstance = &ScriptManagerBridge.CreateNativeManagedInstance,
            CreateUserManagedInstance = &ScriptManagerBridge.CreateUserManagedInstance,
            ReleaseGCHandle = &ScriptManagerBridge.ReleaseGCHandle
        };
    }

    #endregion

    #region Logging

    public static int Log(string message)
    {
        using MingString text = Marshaling.ConvertStringToNative(message);
        return s_callbacks.Log(&text);
    }

    #endregion

    #region Strings

    internal static IntPtr CreateString(byte* str, int length)
    {
        return s_callbacks.CreateString(str, length);
    }

    internal static byte* GetStringBuffer(IntPtr strPtr)
    {
        return s_callbacks.GetStringBuffer(strPtr);
    }

    internal static int GetStringLength(IntPtr strPtr)
    {
        return s_callbacks.GetStringLength(strPtr);
    }

    internal static void DestroyString(IntPtr strPtr)
    {
        s_callbacks.DestroyString(strPtr);
    }

    #endregion

    #region Method Binding

    internal static IntPtr GetMethodBind(string className, string methodName)
    {
        using MingString nativeClass = Marshaling.ConvertStringToNative(className);
        using MingString nativeMethod = Marshaling.ConvertStringToNative(methodName);
        return s_callbacks.GetMethodBind(&nativeClass, &nativeMethod);
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

    internal static delegate* unmanaged<IntPtr> GetConstructor(in MingString name)
    {
        fixed (MingString* namePtr = &name)
        {
            return s_callbacks.GetConstructor(namePtr);
        }
    }

    #endregion

    #region Object Binding

    internal static string GetClassName(IntPtr objPtr)
    {
        using MingString name = default;
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
}
