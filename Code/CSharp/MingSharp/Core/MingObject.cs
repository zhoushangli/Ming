namespace Ming;

using System.Diagnostics;


public partial class MingObject : IDisposable
{

    public class MethodName
    {
        public static readonly string Free = nameof(MingObject.Free);
        public static readonly string _Notification = nameof(MingObject._Notification);
    }

    public class SignalName
    {
    }

    private static readonly Type CachedType = typeof(MingObject);
    private static readonly string NativeName = "Object";

    [DebuggerBrowsable(DebuggerBrowsableState.Never)]
    private unsafe static readonly delegate* unmanaged<IntPtr> NativeCtor = GetConstrcutor(NativeName);

    [DebuggerBrowsable(DebuggerBrowsableState.Never)]
    private static readonly IntPtr FreeMethodBind = NativeFuncs.GetMethodBind("Object", "Free");

    internal IntPtr NativePtr;
    internal long ResourceLease;

    public unsafe MingObject()
    {
        ConstructAndInitialize(NativeCtor, CachedType);
    }

    public unsafe MingObject(nint nativePtr)
    {
        NativePtr = nativePtr;
        ConstructAndInitialize(NativeCtor, CachedType);
    }

    internal MingObject(bool initialize)
    {
    }

    internal static IntPtr GetPtr(MingObject obj)
    {
        if (obj == null)
        {
            return IntPtr.Zero;
        }

        ObjectDisposedException.ThrowIf(obj.NativePtr == IntPtr.Zero, obj);
        return obj.NativePtr;
    }

    internal unsafe static delegate* unmanaged<nint> GetConstrcutor(string name)
    {
        using String nativeName = Marshaling.ConvertStringToNative(name);
        return NativeFuncs.GetConstructor(in nativeName);
    }

    ~MingObject()
    {
        Dispose();
    }

    public void Dispose()
    {
        NativePtr = IntPtr.Zero;
        long lease = System.Threading.Interlocked.Exchange(ref ResourceLease, 0);
        if (lease != 0)
        {
            ScriptManagerBridge.EnqueueResourceRelease(lease);
        }
        GC.SuppressFinalize(this);
    }

    internal void InvalidateFromNative()
    {
        NativePtr = IntPtr.Zero;
        System.Threading.Interlocked.Exchange(ref ResourceLease, 0);
        GC.SuppressFinalize(this);
    }

    // Return the engine class name of this object.
    // e.g. node.GetClassName() -> "Node"
    public string GetClassName()
    {
        return NativeFuncs.GetClassName(GetPtr(this));
    }

    public static T GetGlobalObject<T>(string name) where T : MingObject
    {
        MingObject instance = InteropUtils.UnmanagedGetManaged(NativeFuncs.GetGlobalObject(name));
        if (instance == null)
        {
            throw new InvalidOperationException("Global object is unavailable: " + name);
        }

        return instance as T ?? throw new InvalidCastException("Global object has a different type: " + name);
    }

    internal unsafe void ConstructAndInitialize(delegate* unmanaged<IntPtr> nativeCtor, Type nativeType)
    {
        Type managedType = GetType();
        if (managedType != nativeType)
        {
            if (NativePtr != IntPtr.Zero)
            {
                return;
            }

            throw new NotSupportedException(
                $"User-defined managed type '{managedType.FullName}' is not supported yet."
            );
        }

        if (NativePtr != IntPtr.Zero)
        {
            return;
        }

        NativePtr = nativeCtor();
        InteropUtils.TieManagedToUnmanaged(this, NativePtr);
    }

    public void Free()
    {
        NativeCalls.MingCall_Void(FreeMethodBind, GetPtr(this));
    }

    public virtual void _Notification(int notification)
    {
    }

    public bool EmitSignal(string signalName, params Variant[] arguments)
    {
        return NativeFuncs.EmitSignal(this, signalName, arguments);
    }

    public bool Connect(string signalName, Callable callback)
    {
        return NativeFuncs.ConnectSignal(this, signalName, callback);
    }

    public bool IsConnected(string signalName, Callable callback)
    {
        return NativeFuncs.IsSignalConnected(this, signalName, callback);
    }

    public bool Disconnect(string signalName, Callable callback)
    {
        return NativeFuncs.DisconnectSignal(this, signalName, callback);
    }

    protected internal virtual void InvokeMingClassMethod(in String methodName, NativeVariantPtrArgs args,
        out ming_variant ret, out ming_call_error error)
    {
        ret = default;
        error = new ming_call_error();
        string methodNameStr = methodName.ToString();

        if (methodNameStr == MethodName.Free)
        {
            if (args.Count != 0)
            {
                error.Error = CallErrorCode.TooManyArguments;
                error.ExpectedArgumentCount = 0;
                return;
            }

            Free();
            return;
        }

        if (methodNameStr == MethodName._Notification)
        {
            if (args.Count != 1)
            {
                error.Error = args.Count < 1 ? CallErrorCode.TooFewArguments : CallErrorCode.TooManyArguments;
                error.ExpectedArgumentCount = 1;
                return;
            }

            if (!VariantUtils.TryConvertTo<int>(args[0], 0, Variant.Type.Int, out int notification, out error))
            {
                return;
            }

            _Notification(notification);
            return;
        }

        error.Error = CallErrorCode.InvalidMethod;
    }

    protected internal virtual bool HasMingClassMethod(in String methodName)
    {
        string methodNameStr = methodName.ToString();

        if (methodNameStr == MethodName.Free ||
            methodNameStr == MethodName._Notification)
        {
            return true;
        }

        return false;
    }

}
