namespace Ming;

class PlayerController : Node3D
{
    public string NameSeenInConstructor { get; private set; } = "";

    PlayerController()
    {
        NameSeenInConstructor = GetName();
    }

    public bool ValidateNativeOwner(IntPtr expectedOwner)
    {
        return NativePtr == expectedOwner
            && GetName() == NameSeenInConstructor;
    }
}