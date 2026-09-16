namespace Ming;

using System.Runtime.InteropServices;


// Wrap a native UTF-32 string, own converted strings and borrow callback inputs.
// e.g. using String text = Marshaling.ConvertStringToNative("hello");
[StructLayout(LayoutKind.Sequential)]
public ref struct String : IDisposable
{
    internal ming_string NativeValue;

    // Read the UTF-32 code points without converting them.
    // e.g. Data returns the block the native side wrote into this string.
    internal unsafe IntPtr Data => (IntPtr)NativeValue.Data;

    // Read the code point count that the native block header stores, so no native call is needed.
    // e.g. Length returns 4 for a native String that owns "Ming".
    internal unsafe uint Length => Marshaling.GetStringLength(in NativeValue);

    public unsafe static explicit operator String(string str)
    {
        return Marshaling.ConvertStringToNative(str);
    }

    public unsafe static explicit operator string(String str)
    {
        return str.ToString();
    }

    // Release the reference this string holds on its native block and leave it empty.
    // e.g. Dispose lets the next assignment reuse the same value.
    public unsafe void Dispose()
    {
        if (NativeValue.Data == IntPtr.Zero)
        {
            return;
        }

        // The layout mirrors the native String, so the native destructor can release
        // the buffer through this address and no separate shell object is needed.
        fixed (ming_string* nativeValue = &NativeValue)
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
