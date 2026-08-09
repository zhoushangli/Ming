namespace Ming;

public partial class Node : MingObject
{
    public Node()
        : this(NativeFuncs.CreateObject("Node"))
    {
    }
}
