namespace Ming;

using System.Threading;


// Own a native named call or shared managed delegate state.
// e.g. Callable.From(() => UpdateLabel()) retains the actual callback until its owners release it.
public sealed partial class Callable : IDisposable
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

    public Variant Call(params Variant[] arguments)
    {
        Variant result = Call(arguments, out ming_call_error error);
        if (error.Error != CallErrorCode.Ok)
        {
            NativeFuncs.Log($"[Error] Callable: invocation failed ({error.Error}). Use Call(arguments, out error) for details.");
        }

        return result;
    }

    public Variant Call(Variant[] arguments, out ming_call_error error)
    {
        return NativeFuncs.CallCallable(this, arguments, out error);
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
