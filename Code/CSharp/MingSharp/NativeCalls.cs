using System.Runtime.InteropServices;
using System.Text;

namespace MingSharp
{
    [StructLayout(LayoutKind.Sequential)]
    internal unsafe struct NativeCallbacks
    {
        public delegate* unmanaged<byte*, int, int> LogUtf8;
        public delegate* unmanaged<byte*, IntPtr> CreateObject;
        public delegate* unmanaged<IntPtr, byte*> GetClassName;
    }

    public static unsafe class NativeCalls
    {
        private static NativeCallbacks s_callbacks;

        // Store the native function pointer table passed from the engine.
        // e.g. NativeCalls.Initialize(callbacksPtr, sizeof(NativeCallbacks))
        public static void Initialize(IntPtr callbacks, int size)
        {
            if (size != sizeof(NativeCallbacks))
            {
                throw new InvalidOperationException("Callbacks size mismatch.");
            }

            s_callbacks = *(NativeCallbacks*)callbacks;
        }

        // Forward a UTF-8 log message to the native engine.
        // e.g. NativeCalls.LogUtf8(text, length)
        public static int LogUtf8(byte* text, int length)
        {
            return s_callbacks.LogUtf8(text, length);
        }

        // Create an engine object by class name and return its native pointer.
        // e.g. NativeCalls.CreateObject("Node")
        internal static IntPtr CreateObject(string className)
        {
            byte[] utf8Bytes = Encoding.UTF8.GetBytes(className);

            fixed (byte* text = utf8Bytes)
            {
                return s_callbacks.CreateObject(text);
            }
        }

        // Return the engine class name of the given native object pointer.
        // e.g. NativeCalls.GetClassName(nodePtr) -> "Node"
        internal static string GetClassName(IntPtr objPtr)
        {
            return Marshal.PtrToStringUTF8((nint)s_callbacks.GetClassName(objPtr)) ?? "";
        }
    }
}
