using System.Runtime.InteropServices;

namespace Ming;

[StructLayout(LayoutKind.Sequential)]
public struct Color
{
    public byte R;
    public byte G;
    public byte B;
    public byte A;

    public Color( byte r, byte g, byte b, byte a)
    {
        R = r;
        G = g;
        B = b;
        A = a;
    }

    public Color(byte r, byte g, byte b) : this(r, g, b, 255)
    {
    }

    public override string ToString()
    {
        return $"Color(R={R}, G={G}, B={B}, A={A})";
    }
}