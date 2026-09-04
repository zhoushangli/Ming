namespace Ming;

using System.Diagnostics;


public class MingObject : IDisposable
{
    private static readonly Type CachedType = typeof(MingObject);
    private static readonly string NativeName = "Object";

    [DebuggerBrowsable(DebuggerBrowsableState.Never)]
    private unsafe static readonly delegate* unmanaged<IntPtr> NativeCtor = GetConstrcutor(NativeName);

    internal IntPtr NativePtr;

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
        using MingString mingName = Marshaling.ConvertStringToNative(name);
        return NativeFuncs.GetConstructor((IntPtr)(&mingName));
    }

    public void Dispose()
    {
        if (NativePtr != IntPtr.Zero)
        {
            // NativeFuncs.DestroyManagedScriptInstance(NativePtr);
            NativePtr = IntPtr.Zero;
        }
    }

    // Return the engine class name of this object.
    // e.g. node.GetClassName() -> "Node"
    public string GetClassName()
    {
        return NativeFuncs.GetClassName(GetPtr(this));
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
}
