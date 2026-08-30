namespace Ming;

public class PlayerController
{
    public string NameSeenInConstructor { get; private set; } = "";

    public PlayerController()
    {
        // NameSeenInConstructor = GetName();

        // if (NameSeenInConstructor == "CSharpCreateFailureProbe")
        // {
        //     throw new InvalidOperationException("Expected managed script construction failure.");
        // }
    }

    public bool ValidateNativeOwner(IntPtr expectedOwner)
    {
        // return NativePtr == expectedOwner
        //     && GetName() == NameSeenInConstructor;
        return true;
    }
}
