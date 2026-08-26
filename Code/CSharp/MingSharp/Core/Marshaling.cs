using System.Text;

namespace Ming;

public static class Marshaling
{
    public static unsafe MingString ConvertStringToNative(string? str)
    {
        if (string.IsNullOrEmpty(str))
        {
            return new MingString(IntPtr.Zero);
        }
        
        byte[] utf8Bytes = Encoding.UTF8.GetBytes(str);
        fixed (byte* bytePtr = utf8Bytes)
        {
            IntPtr nativePtr = NativeFuncs.CreateString(bytePtr, utf8Bytes.Length);
            return new MingString(nativePtr);
        }
    }

    public static unsafe string ConvertStringToManaged(in MingString mingString)
    {
        if (mingString.NativePtr == IntPtr.Zero)
        {
            return string.Empty;
        }

        return Encoding.UTF8.GetString(mingString.Buffer, mingString.Length);
    }
}