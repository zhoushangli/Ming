namespace Ming;

using System.Runtime.InteropServices;


// Wrap a native UTF-32 string, own converted strings and borrow callback inputs.
// e.g. using String text = Marshaling.ConvertStringToNative("hello");
[StructLayout(LayoutKind.Sequential)]
public ref struct String : IDisposable
{
    internal InteropTypes.ming_string NativeValue;

    // Read the UTF-32 code points without converting them.
    // e.g. Data returns the buffer the native side wrote into this string.
    internal unsafe uint* Data => NativeValue.Data;

    internal uint Length => NativeValue.Length;

    public unsafe static explicit operator String(string str)
    {
        return Marshaling.ConvertStringToNative(str);
    }

    public unsafe static explicit operator string(String str)
    {
        return str.ToString();
    }

    // Release the UTF-32 buffer owned by this string and leave it empty.
    // e.g. Dispose lets the next assignment reuse the same value.
    public unsafe void Dispose()
    {
        if (NativeValue.Data == null)
        {
            return;
        }

        // The layout mirrors the native String, so the native destructor can release
        // the buffer through this address and no separate shell object is needed.
        fixed (InteropTypes.ming_string* nativeValue = &NativeValue)
        {
            NativeFuncs.DestroyString((String*)nativeValue);
        }

        NativeValue = default;
    }

    public unsafe override string ToString()
    {
        return Marshaling.ConvertStringToManaged(this);
    }
}
