using System.Runtime.InteropServices;
using System.Text;

namespace Ming;

internal unsafe struct UmanagedCallbacks
{
    public delegate* unmanaged<byte*, int, int> LogUtf8;
    public delegate* unmanaged<byte*, IntPtr> CreateObject;
    public delegate* unmanaged<IntPtr, byte*> GetClassName;
    public delegate* unmanaged<byte*, int, byte*, int, IntPtr> GetMethodBind;
    public delegate* unmanaged<IntPtr, IntPtr, void**, void*, void> MethodBindPtrCall;
    public delegate* unmanaged<byte*, int, IntPtr> CreateString;
    public delegate* unmanaged<IntPtr, byte*> GetStringBuffer;
    public delegate* unmanaged<IntPtr, int> GetStringLength;
    public delegate* unmanaged<IntPtr, void> DestroyString;
}

public static unsafe class NativeFuncs
{
    private static UmanagedCallbacks s_callbacks;

    // Store the native function pointer table passed from the engine.
    // e.g. NativeFuncs.Initialize(callbacksPtr, sizeof(NativeCallbacks))
    public static void Initialize(IntPtr callbacks, int size)
    {
        if (size != sizeof(UmanagedCallbacks))
        {
            throw new InvalidOperationException("Callbacks size mismatch.");
        }

        s_callbacks = *(UmanagedCallbacks*)callbacks;
    }

    // Forward a UTF-8 log message to the native engine.
    // e.g. NativeFuncs.LogUtf8(text, length)
    public static int LogUtf8(byte* text, int length)
    {
        return s_callbacks.LogUtf8(text, length);
    }

    // Create an engine object by class name and return its native pointer.
    // e.g. NativeFuncs.CreateObject("Node")
    internal static IntPtr CreateObject(string className)
    {
        byte[] utf8Bytes = Encoding.UTF8.GetBytes(className);

        fixed (byte* text = utf8Bytes)
        {
            return s_callbacks.CreateObject(text);
        }
    }

    // Return the engine class name of the given native object pointer.
    // e.g. NativeFuncs.GetClassName(nodePtr) -> "Node"
    internal static string GetClassName(IntPtr objPtr)
    {
        return Marshal.PtrToStringUTF8((nint)s_callbacks.GetClassName(objPtr)) ?? "";
    }

    internal static IntPtr GetMethodBind(string className, string methodName)
    {
        byte[] classNameBytes = Encoding.UTF8.GetBytes(className);
        byte[] methodNameBytes = Encoding.UTF8.GetBytes(methodName);

        fixed (byte* classNamePtr = classNameBytes)
        {
            fixed (byte* methodNamePtr = methodNameBytes)
            {
                return s_callbacks.GetMethodBind(classNamePtr, classNameBytes.Length, methodNamePtr, methodNameBytes.Length);
            }
        }
    }

    internal static void MethodBindPtrCall(IntPtr methodBind, IntPtr objPtr, void** args, void* retPtr)
    {
        s_callbacks.MethodBindPtrCall(methodBind, objPtr, args, retPtr);
    }

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
}
