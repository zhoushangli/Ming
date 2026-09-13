namespace Ming;

[AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
public sealed class ScriptPathAttribute : Attribute
{
    public string Path { get; }

    public ScriptPathAttribute(string path)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(path);
        Path = path;
    }
}