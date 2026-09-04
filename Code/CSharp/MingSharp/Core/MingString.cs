namespace Ming;

using System.Runtime.InteropServices;
using System.Text;


[StructLayout(LayoutKind.Sequential)]
public ref struct MingString : IDisposable
{
    internal IntPtr NativePtr;

    internal unsafe byte* Buffer
    {
        get
        {
            if (NativePtr == IntPtr.Zero)
            {
                return null;
            }

            return NativeFuncs.GetStringBuffer(NativePtr);
        }
    }

    internal int Length
    {
        get
        {
            if (NativePtr == IntPtr.Zero)
            {
                return 0;
            }

            return NativeFuncs.GetStringLength(NativePtr);
        }
    }

    internal MingString(IntPtr nativePtr)
    {
        NativePtr = nativePtr;
    }

    public unsafe static explicit operator MingString(string str)
    {
        return Marshaling.ConvertStringToNative(str);
    }

    public unsafe static explicit operator string(MingString mingString)
    {
        return mingString.ToString();
    }

    public void Dispose()
    {
        if (NativePtr != IntPtr.Zero)
        {
            NativeFuncs.DestroyString(NativePtr);
            NativePtr = IntPtr.Zero;
        }
    }

    public unsafe override string ToString()
    {
        return Marshaling.ConvertStringToManaged(this);
    }
}
