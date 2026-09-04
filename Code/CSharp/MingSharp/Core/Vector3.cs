namespace Ming;

public struct Vector3 : IEquatable<Vector3>
{
    public float X;
    public float Y; 
    public float Z;

    public Vector3(float x, float y, float z)
    {
        X = x;
        Y = y;
        Z = z;
    }

    public bool Equals(Vector3 other)
    {
        return X == other.X && Y == other.Y && Z == other.Z;
    }

    public override string ToString()
    {
        return $"({X}, {Y}, {Z})";
    }
}
