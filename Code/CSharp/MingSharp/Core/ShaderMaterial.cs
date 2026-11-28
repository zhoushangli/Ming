namespace Ming;

public partial class ShaderMaterial
{
    public bool SetShaderParameter(string name, Variant value)
    {
        using var callback = new Callable(this, "SetShaderParameter");
        using Variant parameterName = Variant.From(name);
        using Variant result = callback.Call(parameterName, value);
        return result.As<bool>();
    }

    public Variant GetShaderParameter(string name)
    {
        using var callback = new Callable(this, "GetShaderParameter");
        using Variant parameterName = Variant.From(name);
        return callback.Call(parameterName);
    }
}
