namespace Ming;

#nullable enable

public partial struct Variant : IDisposable
{
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
        Any = 15,
        UInt32 = 16
    }

    internal sealed class Disposer : IDisposable
    {
        private readonly ming_variant.movable _native;
        private long _lease;

        internal Disposer(in ming_variant value)
        {
            _native = (ming_variant.movable)value;
            _lease = ScriptManagerBridge.RegisterVariant(this, in value);
        }

        ~Disposer()
        {
            Dispose();
        }

        public void Dispose()
        {
            long lease = System.Threading.Interlocked.Exchange(ref _lease, 0);
            if (lease != 0)
            {
                ScriptManagerBridge.EnqueueVariantRelease(lease);
            }

            GC.SuppressFinalize(this);
        }

        internal void InvalidateFromNative()
        {
            System.Threading.Interlocked.Exchange(ref _lease, 0);
            GC.SuppressFinalize(this);
        }

        internal ming_variant BorrowNativeValue()
        {
            ObjectDisposedException.ThrowIf(System.Threading.Interlocked.Read(ref _lease) == 0, this);
            return (ming_variant)_native;
        }
    }

    public Type VariantType
    {
        get
        {
            ming_variant value = BorrowNativeValue();
            return value.Type;
        }
    }

    private ming_variant.movable _native;
    private Disposer? _disposer;

    public static Variant From<T>(T value)
    {
        ScriptManagerBridge.EnsureNativeBindings();
        ming_variant native = VariantUtils.CreateFrom(value);
        return TakeOwnershipOfNative(ref native);
    }

    public T As<T>()
    {
        ming_variant value = BorrowNativeValue();
        try
        {
            return VariantUtils.ConvertTo<T>(in value);
        }
        finally
        {
            GC.KeepAlive(_disposer);
        }
    }

    public Variant Clone()
    {
        ming_variant value = BorrowNativeValue();
        if (_disposer == null)
        {
            return this;
        }

        try
        {
            ming_variant copy = NativeFuncs.CopyVariant(in value);
            return TakeOwnershipOfNative(ref copy);
        }
        finally
        {
            GC.KeepAlive(_disposer);
        }
    }

    public void Dispose()
    {
        _disposer?.Dispose();
        _native = default;
    }

    internal ming_variant BorrowNativeValue()
    {
        return _disposer != null ? _disposer.BorrowNativeValue() : (ming_variant)_native;
    }

    internal static Variant FromNative(in ming_variant value)
    {
        ming_variant copy = NativeFuncs.CopyVariant(in value);
        return TakeOwnershipOfNative(ref copy);
    }

    internal static Variant TakeOwnershipOfNative(ref ming_variant value)
    {
        try
        {
            Variant result = default;
            if (value.Type == Type.String || value.Type == Type.OBB2 || value.Type == Type.Capsule3
                || value.Type == Type.Matrix4x4 || value.OwnsReference)
            {
                result._disposer = new Disposer(in value);
            }
            else
            {
                result._native = (ming_variant.movable)value;
            }

            value = default;
            return result;
        }
        catch
        {
            NativeFuncs.DestroyVariant(ref value);
            throw;
        }
    }
}
