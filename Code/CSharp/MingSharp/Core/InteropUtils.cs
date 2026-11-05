namespace Ming;

using System.Runtime.InteropServices;


internal static class InteropUtils
{
    public static void TieManagedToUnmanaged(MingObject managed, IntPtr unmanaged)
    {
        IntPtr gcHandlePtr = ScriptManagerBridge.AllocNativeBindingGCHandle(managed);
        bool transferred = false;

        try
        {
            if (!NativeFuncs.TieNativeManagedToUnmanaged(gcHandlePtr, unmanaged))
            {
                throw new InvalidOperationException(
                    "Failed to tie the native managed wrapper to its owner."
                );
            }

            transferred = true;
        }

        finally
        {
            if (!transferred)
            {
                if (managed is RefCounted)
                {
                    managed.Dispose();
                }
                else
                {
                    ScriptManagerBridge.ReleaseGCHandleCore(gcHandlePtr);
                }
            }
        }
    }

    public static MingObject UnmanagedGetManaged(IntPtr unmanaged)
    {
        if (unmanaged == IntPtr.Zero)
        {
            return null;
        }

        IntPtr gcHandlePtr = NativeFuncs.GetScriptInstanceManaged(unmanaged, out bool hasScript);
        if (hasScript)
        {
            return gcHandlePtr != IntPtr.Zero ? GCHandle.FromIntPtr(gcHandlePtr).Target as MingObject : null;
        }

        gcHandlePtr = NativeFuncs.UnmanagedGetInstanceBindingManaged(unmanaged);
        if (gcHandlePtr != IntPtr.Zero && GCHandle.FromIntPtr(gcHandlePtr).Target is MingObject wrapper
            && wrapper.NativePtr != IntPtr.Zero)
        {
            return wrapper;
        }

        gcHandlePtr = NativeFuncs.UnmanagedInstanceBindingCreateManaged(unmanaged);
        return gcHandlePtr != IntPtr.Zero
            ? GCHandle.FromIntPtr(gcHandlePtr).Target as MingObject
            : null;
    }
}
