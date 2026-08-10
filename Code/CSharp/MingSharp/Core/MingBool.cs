namespace Ming;

// Bool is very special, for different platform, the size of bool is different
// e.g. in x64 bool is 1 byte, but BOOL in Windows is 4 bytes, so we need to use a custom enum to represent bool in Ming
public enum MingBool : byte
{
    False = 0,
    True = 1
}

public static class MingBoolExtensions
{
    public static bool ToBool(this MingBool mingBool)
    {
        return mingBool == MingBool.True;
    }

    public static MingBool ToMingBool(this bool value)
    {
        return value ? MingBool.True : MingBool.False;
    }
}
