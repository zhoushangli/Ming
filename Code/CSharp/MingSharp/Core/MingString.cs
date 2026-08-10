using System.Runtime.InteropServices;
using System.Text;

namespace Ming;

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

    public unsafe static implicit operator MingString(string str)
    {
        byte[] utf8Bytes = Encoding.UTF8.GetBytes(str);
        fixed (byte* bytePtr = utf8Bytes)
        {
            IntPtr nativePtr = NativeFuncs.CreateString(bytePtr, utf8Bytes.Length);
            return new MingString(nativePtr);
        }
    }

    public unsafe static implicit operator string(MingString mingString)
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
        if (NativePtr == IntPtr.Zero)
        {
            return string.Empty;
        }

        return Encoding.UTF8.GetString(Buffer, Length);
    }
}