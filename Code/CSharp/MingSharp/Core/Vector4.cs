
namespace Ming;

public struct Vector4 : IEquatable<Vector4>
{
    public float X;
    public float Y; 
    public float Z;
    public float W;

    public Vector4(float x, float y, float z, float w)
    {
        X = x;
        Y = y;
        Z = z;
        W = w;
    }

    public bool Equals(Vector4 other)
    {
        return X == other.X && Y == other.Y && Z == other.Z && W == other.W;
    }

    public override string ToString()
    {
        return $"({X}, {Y}, {Z}, {W})";
    }
}