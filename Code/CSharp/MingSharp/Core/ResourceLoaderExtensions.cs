namespace Ming;

public static partial class ResourceLoader
{
    public static T Load<T>(string path) where T : Resource
    {
        return (T)Load(path);
    }
}
