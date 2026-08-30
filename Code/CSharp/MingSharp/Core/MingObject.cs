using System.Diagnostics;

namespace Ming;

public class MingObject : IDisposable
{
    private static readonly string NativeName = "Object";

    [DebuggerBrowsable(DebuggerBrowsableState.Never)]
    private unsafe static readonly delegate* unmanaged<IntPtr> NativeCtor = GetConstrcutor(NativeName);

    internal IntPtr NativePtr;

    internal MingObject(bool initialize)
    {
    }

    internal static IntPtr GetPtr(MingObject? obj)
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

    internal unsafe void ConstructAndInitialize(delegate* unmanaged<IntPtr> nativeCtor, string nativeName, Type nativeType)
    {
        if (NativePtr == IntPtr.Zero)
        {
            NativePtr = nativeCtor();
            // InteropUtils.TieManagedToUnmanaged(this, NativePtr, nativeName, GetType(), cachedType);
        }
        else
        {
            // InteropUtils.TieManagedToUnmanagedWithPreSetup(this, NativePtr, GetType(), cachedType);
        }
    }
}
