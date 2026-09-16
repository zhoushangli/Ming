namespace Ming;

public partial struct Variant
{
    internal ming_variant.movable NativeVariant;

    public enum Type : int
    {
        Empty = 0,
        Bool = 1,
        Int = 2,
        Float = 3,
        String = 4,
        Vector2 = 5,
        Vector3 = 6,
        Vector4 = 7,
        Color = 8,
        AABB2 = 9,
        OBB2 = 10,
        Capsule3 = 11,
        EulerAngles = 12,
        Matrix4x4 = 13,
        ObjectPtr = 14,
        Any = 15
    }


}
