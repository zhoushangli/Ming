namespace MingSharp;

public partial class Node : MingObject
{
    public Node()
        : this(NativeFuncs.CreateObject("Node"))
    {
    }

    public bool GetProcess()
    {
        if (GetProcessMethodBind == IntPtr.Zero)
        {
            throw new InvalidOperationException("MethodBind not found: Node.GetProcess");
        }

        return NativeCalls.CallBool0(GetProcessMethodBind, GetPtr(this));
    }
}
