namespace Ming;

public class PlayerController : Node3D
{
    public string NameSeenInConstructor { get; private set; } = "";

    public PlayerController()
    {
        NameSeenInConstructor = GetName();
    }

    public bool ValidateNativeOwner(IntPtr expectedOwner)
    {
        // return NativePtr == expectedOwner
        //     && GetName() == NameSeenInConstructor;
        return true;
    }
}
