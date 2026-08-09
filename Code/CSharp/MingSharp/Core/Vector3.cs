
namespace Ming;

public struct Vector3 : IEquatable<Vector3>
{
    public float X;
    public float Y; 
    public float Z;

    public bool Equals(Vector3 other)
    {
        return X == other.X && Y == other.Y && Z == other.Z;
    }
}