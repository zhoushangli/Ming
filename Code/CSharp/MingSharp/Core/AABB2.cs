namespace Ming;

using System.Runtime.InteropServices;


[StructLayout(LayoutKind.Sequential)]
public struct AABB2
{
    public Vector2 Mins;
    public Vector2 Maxs;

    public AABB2(Vector2 mins, Vector2 maxs)
    {
        Mins = mins;
        Maxs = maxs;
    }

    public AABB2(float minX, float minY, float maxX, float maxY)
    {
        Mins = new Vector2(minX, minY);
        Maxs = new Vector2(maxX, maxY);
    }

    public override string ToString()
    {
        return $"AABB2(Mins={Mins}, Maxs={Maxs})";
    }
}
