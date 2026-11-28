namespace Ming;

using System.Runtime.InteropServices;


[StructLayout(LayoutKind.Sequential)]
public struct Color
{
    public float R;
    public float G;
    public float B;
    public float A;

    public Color(float r, float g, float b, float a)
    {
        R = r;
        G = g;
        B = b;
        A = a;
    }

    public Color(float r, float g, float b) : this(r, g, b, 1f)
    {
    }

    public override string ToString()
    {
        return $"Color(R={R}, G={G}, B={B}, A={A})";
    }
}