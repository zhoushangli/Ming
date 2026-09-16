namespace Ming;

public static partial class VariantUtils
{
    // Read the native pointer out of a variant that holds an object.
    // e.g. ConvertToMingObjectPtr(variant) returns IntPtr.Zero for a null object pointer.
    public static unsafe IntPtr ConvertToMingObjectPtr(in ming_variant variant)
    {
        GuaranteeTag(in variant, Variant.Type.ObjectPtr);

        return variant.ObjectPtr == null ? IntPtr.Zero : (IntPtr)variant.ObjectPtr;
    }

    // Read the stored value as T, leaving the variant and everything it owns untouched.
    // e.g. ConvertTo<Vector3>(variant) returns a copy of the inline Vector3.
    public static unsafe T ConvertTo<T>(in ming_variant variant)
    {
        if (typeof(T) == typeof(bool))
        {
            GuaranteeTag(in variant, Variant.Type.Bool);
            return (T)(object)variant.Bool.ToBool();
        }

        if (typeof(T) == typeof(int))
        {
            GuaranteeTag(in variant, Variant.Type.Int);
            return (T)(object)variant.Int;
        }

        if (typeof(T) == typeof(float))
        {
            GuaranteeTag(in variant, Variant.Type.Float);
            return (T)(object)variant.Float;
        }

        if (typeof(T) == typeof(string))
        {
            GuaranteeTag(in variant, Variant.Type.String);

            // The variant owns the text block, so the managed copy must not release it.
            ming_string nativeString = variant.String;
            return (T)(object)Marshaling.ConvertStringToManaged(in nativeString);
        }

        if (typeof(T) == typeof(Vector2))
        {
            GuaranteeTag(in variant, Variant.Type.Vector2);
            return (T)(object)variant.Vector2;
        }

        if (typeof(T) == typeof(Vector3))
        {
            GuaranteeTag(in variant, Variant.Type.Vector3);
            return (T)(object)variant.Vector3;
        }

        if (typeof(T) == typeof(Vector4))
        {
            GuaranteeTag(in variant, Variant.Type.Vector4);
            return (T)(object)variant.Vector4;
        }

        if (typeof(T) == typeof(Color))
        {
            GuaranteeTag(in variant, Variant.Type.Color);
            return (T)(object)variant.Color;
        }

        if (typeof(T) == typeof(AABB2))
        {
            GuaranteeTag(in variant, Variant.Type.AABB2);
            return (T)(object)variant.AABB2;
        }

        if (typeof(T) == typeof(EulerAngles))
        {
            GuaranteeTag(in variant, Variant.Type.EulerAngles);
            return (T)(object)variant.EulerAngles;
        }

        if (typeof(T) == typeof(OBB2))
        {
            GuaranteeTag(in variant, Variant.Type.OBB2);
            return (T)(object)ReadOwned<OBB2>(in variant);
        }

        if (typeof(T) == typeof(Capsule3))
        {
            GuaranteeTag(in variant, Variant.Type.Capsule3);
            return (T)(object)ReadOwned<Capsule3>(in variant);
        }

        if (typeof(T) == typeof(Matrix4x4))
        {
            GuaranteeTag(in variant, Variant.Type.Matrix4x4);
            return (T)(object)ReadOwned<Matrix4x4>(in variant);
        }

        if (typeof(MingObject).IsAssignableFrom(typeof(T)))
        {
            object managedObject = InteropUtils.UnmanagedGetManaged(ConvertToMingObjectPtr(in variant));

            // A variant can hold a null object pointer, which is not an error.
            if (managedObject is null)
            {
                return default;
            }

            if (managedObject is not T result)
            {
                throw new InvalidCastException(
                    $"Cannot read Variant of type {variant.Type} as {typeof(T).Name}, "
                    + $"the stored object is a {managedObject.GetType().Name}.");
            }

            return result;
        }

        throw new NotSupportedException($"Variant cannot be converted to {typeof(T).FullName}.");
    }

    // Reject a variant whose tag is not the one the caller asked for.
    // e.g. RequireTag on a Variant holding an Int throws when a Float was requested.
    private static void GuaranteeTag(in ming_variant variant, Variant.Type expected)
    {
        if (variant.Type != expected)
        {
            throw new InvalidCastException($"Cannot read Variant of type {variant.Type} as {expected}.");
        }
    }

    // Copy the value that the variant owns behind its payload pointer.
    // e.g. ReadOwned<Matrix4x4>(variant) returns a copy that outlives the variant.
    private static unsafe T ReadOwned<T>(in ming_variant variant)
        where T : unmanaged
    {
        void* ownedValue = variant.OwnedValue;
        if (ownedValue == null)
        {
            throw new InvalidCastException(
                $"Variant of type {variant.Type} owns no value, so it cannot be read as {typeof(T).Name}.");
        }

        return *(T*)ownedValue;
    }
}