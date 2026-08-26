namespace Ming;

public partial class Node3D : Node
{
    public Node3D() : base(NativeFuncs.CreateObject("Node3D"))
    {
    }

}

public partial class OmniLight3D : Light3D
{
    public OmniLight3D() : base(NativeFuncs.CreateObject("OmniLight3D"))
    {
    }

}