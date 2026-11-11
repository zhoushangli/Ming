namespace Ming;

using System.Threading;


// Describe a named call without retaining its target object or allocating a delegate handle.
// e.g. new Callable(controller, nameof(OnButtonPressed)) identifies a script callback.
public sealed class Callable : IDisposable
{
    private ming_callable _nativeValue;
    private long _nativeLease;

    public Callable(MingObject target, string methodName)
    {
        ArgumentNullException.ThrowIfNull(target);
        ArgumentException.ThrowIfNullOrEmpty(methodName);

        ming_callable value = NativeFuncs.CreateCallable(MingObject.GetPtr(target), methodName);
        GC.KeepAlive(target);
        TakeOwnership(ref value);
    }

    private Callable(ref ming_callable value)
    {
        TakeOwnership(ref value);
    }

    ~Callable()
    {
        Dispose();
    }

    // Create an independent owner from a borrowed native value through the native copy constructor.
    // e.g. FromNative(in source) remains valid after the original native Callable is destroyed.
    internal static Callable FromNative(in ming_callable source)
    {
        ming_callable value = NativeFuncs.CopyCallable(in source);
        return new Callable(ref value);
    }

    // Move an owned native return value into a wrapper and empty the caller's storage.
    // e.g. TakeOwnershipOfNative(ref result) transfers rather than duplicates result's reference.
    internal static Callable TakeOwnershipOfNative(ref ming_callable value)
    {
        return new Callable(ref value);
    }

    public void Dispose()
    {
        long lease = Interlocked.Exchange(ref _nativeLease, 0);
        _nativeValue = default;
        if (lease != 0)
        {
            ScriptManagerBridge.EnqueueCallableRelease(lease);
        }

        GC.SuppressFinalize(this);
    }

    // Borrow the layout only for a synchronous native operation; the registry retains ownership.
    // e.g. Connect borrows this value while the native connection makes an owning copy.
    internal ming_callable BorrowNativeValue()
    {
        ObjectDisposedException.ThrowIf(Interlocked.Read(ref _nativeLease) == 0, this);
        return _nativeValue;
    }

    internal void InvalidateFromNative()
    {
        Interlocked.Exchange(ref _nativeLease, 0);
        _nativeValue = default;
        GC.SuppressFinalize(this);
    }

    private void TakeOwnership(ref ming_callable value)
    {
        try
        {
            _nativeLease = ScriptManagerBridge.RegisterCallable(this, in value);
            _nativeValue = value;
            value = default;
        }
        catch
        {
            NativeFuncs.DestroyCallable(ref value);
            throw;
        }
    }
}
