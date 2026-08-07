using System.Runtime.InteropServices;
using System.Text;

namespace MingSharp
{
    internal static unsafe class NativeCalls
    {
        // Forward a UTF-8 log message to the native engine.
        // e.g. NativeCalls.LogUtf8(text, length)
        public static int LogUtf8(byte* text, int length)
        {
            return NativeFuncs.LogUtf8(text, length);
        }

        // Create an engine object by class name and return its native pointer.
        // e.g. NativeCalls.CreateObject("Node")
        internal static IntPtr CreateObject(string className)
        {
            return NativeFuncs.CreateObject(className);
        }

        // Return the engine class name of the given native object pointer.
        // e.g. NativeCalls.GetClassName(nodePtr) -> "Node"
        internal static string GetClassName(IntPtr objPtr)
        {
            return NativeFuncs.GetClassName(objPtr);
        }

        internal static IntPtr GetMethodBind(string className, string methodName)
        {
            return NativeFuncs.GetMethodBind(className, methodName);
        }

        internal static bool CallBool0(IntPtr methodBind, IntPtr objectPtr)
        {
            int ret = 0;
            NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, null, &ret);
            return ret != 0;
        }
    }
}
