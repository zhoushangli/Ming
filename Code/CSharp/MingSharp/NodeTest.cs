namespace MingSharp;

public partial class Node : MingObject
{
    public Node()
        : this(NativeFuncs.CreateObject("Node"))
    {
    }
}
