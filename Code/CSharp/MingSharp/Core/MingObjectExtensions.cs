namespace Ming;

public partial class MingObject
{
    public static void Log(string message)
    {
        NativeFuncs.Log(message);
    }
}
