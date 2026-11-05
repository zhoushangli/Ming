namespace Ming;

public partial class Node
{
    public T GetNode<T>(string path) where T : Node
    {
        return (T)GetNode(path);
    }

    public T GetNodeOrNull<T>(string path) where T : Node
    {
        return GetNodeOrNull(path) as T;
    }
}
