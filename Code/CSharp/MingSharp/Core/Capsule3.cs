namespace Ming;

using System.Runtime.InteropServices;


// Capsule3 is 28 bytes, so a Variant keeps it behind an owned pointer instead of inline.
[StructLayout(LayoutKind.Sequential)]
public struct Capsule3
{
    public Vector3 Start;
    public Vector3 End;
    public float Radius;

    public Capsule3(Vector3 start, Vector3 end, float radius)
    {
        Start = start;
        End = end;
        Radius = radius;
    }

    public override string ToString()
    {
        return $"Capsule3(Start={Start}, End={End}, Radius={Radius})";
    }
}
