namespace Ming;

using System.Runtime.InteropServices;
using System.Text;


public static class Marshaling
{
    // Convert a managed string into an owned native UTF-32 string.
    // e.g. using String text = Marshaling.ConvertStringToNative("hello") frees it on Dispose.
    public static unsafe String ConvertStringToNative(string str)
    {
        String result = default;

        if (string.IsNullOrEmpty(str))
        {
            return result;
        }

        // 1) Encode the managed text into UTF-32 code points
        // 2) Let the native side copy the code points into the result
        byte[] utf32Bytes = Encoding.UTF32.GetBytes(str);
        ReadOnlySpan<uint> codePoints = MemoryMarshal.Cast<byte, uint>(utf32Bytes);

        fixed (uint* codePointPtr = codePoints)
        {
            NativeFuncs.CreateString(codePointPtr, codePoints.Length, &result);
        }

        return result;
    }

    // Convert a native UTF-32 string into a managed string without taking ownership.
    // e.g. Marshaling.ConvertStringToManaged(name) leaves name owned by its holder.
    public static unsafe string ConvertStringToManaged(in String str)
    {
        uint length = str.Length;

        if (length == 0)
        {
            return string.Empty;
        }

        return Encoding.UTF32.GetString((byte*)str.Data, checked((int)(length * sizeof(uint))));
    }
}
