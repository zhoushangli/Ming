namespace Ming;

using System.Runtime.InteropServices;


// Describe the raw memory layout of the native types exchanged across the bridge.
// e.g. InteropTypes.ming_string mirrors the native String declared in Core/String.hpp.
public static class InteropTypes
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct ming_string
    {
        public uint* Data;
        public uint Length;
    }
}
