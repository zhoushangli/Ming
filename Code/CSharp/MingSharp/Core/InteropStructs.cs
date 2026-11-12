namespace Ming;

using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

public enum CallErrorCode : int
{
    Ok = 0,
    InvalidTarget = 1,
    InvalidMethod = 2,
    TooFewArguments = 3,
    TooManyArguments = 4,
    InvalidArgument = 5,
    ExecutionFailed = 6,
}

[StructLayout(LayoutKind.Sequential)]
public struct ming_call_error
{
    public CallErrorCode Error;
    public int Argument;
    public int ExpectedArgumentCount;
    public Variant.Type ExpectedType;

    public ming_call_error()
    {
        Error = CallErrorCode.Ok;
        Argument = -1;
        ExpectedArgumentCount = 0;
        ExpectedType = Variant.Type.Empty;
    }
}

// Bool is very special, for different platform, the size of bool is different
// e.g. in x64 bool is 1 byte, but BOOL in Windows is 4 bytes, so we need to use a custom enum to represent bool in Ming
public enum ming_bool : byte
{
    False = 0,
    True = 1
}

public static class ming_bool_extensions
{
    public static bool ToBool(this ming_bool mingBool)
    {
        return mingBool == ming_bool.True;
    }

    public static ming_bool ToMingBool(this bool value)
    {
        return value ? ming_bool.True : ming_bool.False;
    }
}

[StructLayout(LayoutKind.Sequential)]
internal ref struct ming_ref
{
    internal IntPtr Reference;

    public void Dispose()
    {
        IntPtr pointer = Reference;
        Reference = IntPtr.Zero;

        if (pointer != IntPtr.Zero)
        {
            NativeFuncs.ReleaseRef(pointer);
        }
    }
}

// Pack = 8 will align the 16 byte payload of ming_variant to 8 bytes
// We use ref here to prevent ming_variant hold by struct or class for long term usage
// Because normally it is passing as a temporary value
[StructLayout(LayoutKind.Sequential, Pack = 8)]
public unsafe ref struct ming_variant
{
    [StructLayout(LayoutKind.Explicit, Size = 16)]
    public unsafe struct ming_variant_payload
    {
        [FieldOffset(0)] public ming_bool Bool;
        [FieldOffset(0)] public int Int;
        [FieldOffset(0)] public uint UInt32;
        [FieldOffset(0)] public float Float;
        [FieldOffset(0)] public Color Color;
        [FieldOffset(0)] public Vector2 Vector2;
        [FieldOffset(0)] public Vector3 Vector3;
        [FieldOffset(0)] public Vector4 Vector4;

        // AABB2 is 16 bytes, so it fits the payload and stays inline.
        // e.g. AABB2(Vector2.Zero, Vector2.One) is stored without an allocation.
        [FieldOffset(0)] public AABB2 AABB2;

        [FieldOffset(0)] public EulerAngles EulerAngles;
        [FieldOffset(0)] public ming_string String;
        [FieldOffset(0)] public void* ObjectPtr;

        // OBB2, Capsule3 and Matrix4x4 are wider than the payload, so they live in an owned block.
        // e.g. OwnedValue points at an owned Matrix4x4 when the variant type is Matrix4x4.
        [FieldOffset(0)] public void* OwnedValue;
    }

    // Sometimes, we might want to hold a ming_variant in a struct or class for long term usage
    // So we need to make it movable
#pragma warning disable CS8981 // The type name only contains lower-cased ascii characters
    [StructLayout(LayoutKind.Sequential, Pack = 8)]
    internal struct movable
#pragma warning restore CS8981
    {
        private int _type;
        private uint _ownsRef;

        private ming_variant_payload _payload;

        public static explicit operator movable(in ming_variant value)
        {
            return new movable
            {
                _type = value._type,
                _ownsRef = value._ownsRef,
                _payload = value._payload
            };
        }

        public static explicit operator ming_variant(movable value)
        {
            return new ming_variant
            {
                _type = value._type,
                _ownsRef = value._ownsRef,
                _payload = value._payload
            };
        }
    }

    internal readonly bool OwnsReference => _ownsRef != 0;

    public Variant.Type Type
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => (Variant.Type)_type;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _type = (int)value;
    }

    public ming_bool Bool
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.Bool;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.Bool = value;
    }

    public int Int
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.Int;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.Int = value;
    }

    public uint UInt32
    {
        readonly get => _payload.UInt32;
        set => _payload.UInt32 = value;
    }

    public float Float
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.Float;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.Float = value;
    }

    public Color Color
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.Color;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.Color = value;
    }

    public Vector2 Vector2
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.Vector2;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.Vector2 = value;
    }

    public Vector3 Vector3
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.Vector3;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.Vector3 = value;
    }

    public Vector4 Vector4
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.Vector4;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.Vector4 = value;
    }

    public AABB2 AABB2
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.AABB2;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.AABB2 = value;
    }

    public EulerAngles EulerAngles
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.EulerAngles;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.EulerAngles = value;
    }

    public ming_string String
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.String;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.String = value;
    }

    public void* ObjectPtr
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.ObjectPtr;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.ObjectPtr = value;
    }

    public void* OwnedValue
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        readonly get => _payload.OwnedValue;
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        set => _payload.OwnedValue = value;
    }

    private int _type;
    private uint _ownsRef;
    private ming_variant_payload _payload;

    public void Dispose()
    {
        NativeFuncs.DestroyVariant(ref this);
    }
}

// Mirror native Callable storage; raw layout assignment borrows rather than retains its payload.
// e.g. CopyCallable creates an owner, while BorrowNativeValue only copies these three fields.
[StructLayout(LayoutKind.Sequential)]
internal struct ming_callable
{
    public ulong ObjectID;
    public ming_string Method;
    public IntPtr Managed;
}

// Point at the code points owned by a native String, which lives in a native CowData block.
// e.g. ming_string.Data is the first code point, or null when the native string is empty.
[StructLayout(LayoutKind.Sequential)]
public unsafe struct ming_string
{
    public IntPtr Data;
}

// Describe the native CowData header that sits right before the code points of a block.
// e.g. ((ming_string_header*)Data)[-1].Size is the code point count of that string.
[StructLayout(LayoutKind.Sequential, Size = 16)]
public struct ming_string_header
{
    public uint RefCount;
    public uint Size;
    public uint Capacity;
}
