namespace Ming;

using System.Runtime.InteropServices;


// OBB2 is 24 bytes, so a Variant keeps it behind an owned pointer instead of inline.
[StructLayout(LayoutKind.Sequential)]
public struct OBB2
{
    public Vector2 Center;
    public Vector2 IBasisNormal;
    public Vector2 HalfDimensions;

    public OBB2(Vector2 center, Vector2 iBasisNormal, Vector2 halfDimensions)
    {
        Center = center;
        IBasisNormal = iBasisNormal;
        HalfDimensions = halfDimensions;
    }

    public override string ToString()
    {
        return $"OBB2(Center={Center}, IBasisNormal={IBasisNormal}, HalfDimensions={HalfDimensions})";
    }
}
