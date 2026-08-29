using System.Runtime.InteropServices;

namespace Ming;

public class MingObject : IDisposable
{
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

    internal void ConstructAndInitialize(string nativeName, Type nativeType)
    {
        if (NativePtr == nint.Zero)
        {
            NativePtr = NativeFuncs.CreateObject(nativeName);

            // 下一阶段：
            // BindManagedToNewNative(this, NativePtr, nativeType);
        }
        else if (GetType() != nativeType)
        {
            GCHandle handle = GCHandle.Alloc(this, GCHandleType.Normal);
            IntPtr handlePtr = GCHandle.ToIntPtr(handle);

            if (!NativeFuncs.BindManagedScriptInstance(NativePtr, handlePtr))
            {
                handle.Free();

                throw new InvalidOperationException(
                    "Failed to bind managed script instance."
                );
            }

            // Native owns handlePtr after a successful callback.
        }
    }
}
