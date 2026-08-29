using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;

namespace Ming;

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct NativeCallbacks
{
    public delegate* unmanaged<byte*, int, int> LogUtf8;
    public delegate* unmanaged<byte*, IntPtr> CreateObject;
    public delegate* unmanaged<IntPtr, byte*> GetClassName;
    public delegate* unmanaged<byte*, int, byte*, int, IntPtr> GetMethodBind;
    public delegate* unmanaged<IntPtr, IntPtr, void**, void*, void> MethodBindPtrCall;
    public delegate* unmanaged<byte*, int, IntPtr> CreateString;
    public delegate* unmanaged<IntPtr, byte*> GetStringBuffer;
    public delegate* unmanaged<IntPtr, int> GetStringLength;
    public delegate* unmanaged<IntPtr, void> DestroyString;
    public delegate* unmanaged<IntPtr, IntPtr, int> BindManagedScriptInstance;
}

[StructLayout(LayoutKind.Sequential)]
internal unsafe struct ManagedCallbacks
{
    public delegate* unmanaged<int> Ping;
    public delegate* unmanaged<IntPtr> CreateTestGCHandle;
    public delegate* unmanaged<IntPtr, int> InvokeTestGCHandle;
    public delegate* unmanaged<IntPtr, void> FreeGCHandle;
    public delegate* unmanaged<int*, int*, int*, void> CollectAndGetState;
    public delegate* unmanaged<IntPtr, IntPtr, int> CreateManagedScriptInstance;
    public delegate* unmanaged<IntPtr, IntPtr, int> ValidateManagedScriptInstance;
    public delegate* unmanaged<int*, int*, int*, int*, void> CollectAndGetManagedScriptState;
}

public static unsafe class NativeFuncs
{
    private static NativeCallbacks s_callbacks;

    private sealed class GCHandleProbe
    {
        private int _invokeCount = 0;

        public int Invoke()
        {
            return ++_invokeCount;
        }
    }

    private static int s_allocatedHandleCount;
    private static int s_freedHandleCount;
    private static WeakReference<GCHandleProbe>? s_lastProbe;

    private static int s_scriptHandleAllocated;
    private static int s_scriptHandleDisposed;
    private static int s_scriptHandleFreed;

    private static WeakReference<PlayerController>? s_lastScriptInstance;

    #region Native Callback Wrappers

    // Forward a UTF-8 log message to the native engine.
    // e.g. NativeFuncs.LogUtf8(text, length)
    public static int LogUtf8(byte* text, int length)
    {
        return s_callbacks.LogUtf8(text, length);
    }

    // Create an engine object by class name and return its native pointer.
    // e.g. NativeFuncs.CreateObject("Node")
    internal static IntPtr CreateObject(string className)
    {
        byte[] utf8Bytes = Encoding.UTF8.GetBytes(className);

        fixed (byte* text = utf8Bytes)
        {
            return s_callbacks.CreateObject(text);
        }
    }

    // Return the engine class name of the given native object pointer.
    // e.g. NativeFuncs.GetClassName(nodePtr) -> "Node"
    internal static string GetClassName(IntPtr objPtr)
    {
        return Marshal.PtrToStringUTF8((nint)s_callbacks.GetClassName(objPtr)) ?? "";
    }

    internal static IntPtr GetMethodBind(string className, string methodName)
    {
        byte[] classNameBytes = Encoding.UTF8.GetBytes(className);
        byte[] methodNameBytes = Encoding.UTF8.GetBytes(methodName);

        fixed (byte* classNamePtr = classNameBytes)
        {
            fixed (byte* methodNamePtr = methodNameBytes)
            {
                return s_callbacks.GetMethodBind(
                    classNamePtr,
                    classNameBytes.Length,
                    methodNamePtr,
                    methodNameBytes.Length
                );
            }
        }
    }

    internal static void MethodBindPtrCall(
        IntPtr methodBind,
        IntPtr objPtr,
        void** args,
        void* retPtr
    )
    {
        s_callbacks.MethodBindPtrCall(methodBind, objPtr, args, retPtr);
    }

    internal static IntPtr CreateString(byte* str, int length)
    {
        return s_callbacks.CreateString(str, length);
    }

    internal static byte* GetStringBuffer(IntPtr strPtr)
    {
        return s_callbacks.GetStringBuffer(strPtr);
    }

    internal static int GetStringLength(IntPtr strPtr)
    {
        return s_callbacks.GetStringLength(strPtr);
    }

    internal static void DestroyString(IntPtr strPtr)
    {
        s_callbacks.DestroyString(strPtr);
    }

    internal static bool BindManagedScriptInstance(IntPtr owner, IntPtr gcHandle)
    {
        if (owner == IntPtr.Zero || gcHandle == IntPtr.Zero)
        {
            return false;
        }

        return s_callbacks.BindManagedScriptInstance(owner, gcHandle) != 0;
    }

    internal static void BindManagedScriptInstance(MingObject instance, IntPtr owner)
    {
        GCHandle handle = GCHandle.Alloc(instance, GCHandleType.Normal);
        IntPtr handlePtr = GCHandle.ToIntPtr(handle);
        bool transferred = false;

        s_scriptHandleAllocated++;

        try
        {
            if (!BindManagedScriptInstance(owner, handlePtr))
            {
                throw new InvalidOperationException(
                    "Failed to bind managed script instance."
                );
            }

            transferred = true;
        }
        finally
        {
            if (!transferred)
            {
                handle.Free();
                s_scriptHandleFreed++;
            }
        }
    }

    #endregion

    #region Managed Callback Wrappers

    [UnmanagedCallersOnly]
    internal static int Ping()
    {
        return 42; // Arbitrary value to indicate the managed code is alive.
    }

    [UnmanagedCallersOnly]
    private static IntPtr CreateTestGCHandle()
    {
        try
        {
            GCHandleProbe probe = new();

            s_lastProbe = new WeakReference<GCHandleProbe>(probe);

            GCHandle handle = GCHandle.Alloc(probe, GCHandleType.Normal);

            s_allocatedHandleCount++;

            return GCHandle.ToIntPtr(handle);
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return IntPtr.Zero;
        }
    }

    [UnmanagedCallersOnly]
    private static int InvokeTestGCHandle(IntPtr handlePtr)
    {
        try
        {
            if (handlePtr == IntPtr.Zero)
            {
                return -1;
            }

            GCHandle handle = GCHandle.FromIntPtr(handlePtr);

            if (handle.Target is not GCHandleProbe probe)
            {
                return -2;
            }

            return probe.Invoke();
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return -3;
        }
    }

    [UnmanagedCallersOnly]
    private static void FreeGCHandle(IntPtr handlePtr)
    {
        try
        {
            if (handlePtr == IntPtr.Zero)
            {
                return;
            }

            GCHandle handle = GCHandle.FromIntPtr(handlePtr);

            bool isScriptInstance = handle.Target is PlayerController;

            if (handle.Target is MingObject instance)
            {
                instance.Dispose();

                if (isScriptInstance)
                {
                    if (instance.NativePtr == IntPtr.Zero)
                    {
                        s_scriptHandleDisposed++;
                    }
                    else
                    {
                        Console.Error.WriteLine(
                            "Managed script NativePtr was not invalidated."
                        );
                    }
                }
            }

            handle.Free();

            if (isScriptInstance)
            {
                s_scriptHandleFreed++;
            }
            else
            {
                s_freedHandleCount++;
            }
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
        }
    }

    [UnmanagedCallersOnly]
    private static unsafe void CollectAndGetState(int* allocated, int* freed, int* targetAlive)
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();

        *allocated = s_allocatedHandleCount;
        *freed = s_freedHandleCount;
        *targetAlive = s_lastProbe?.TryGetTarget(out _) == true ? 1 : 0;
    }

    [UnmanagedCallersOnly]
    private static int CreateManagedScriptInstance(IntPtr scriptPtr, IntPtr ownerPtr)
    {
        if (scriptPtr == IntPtr.Zero || ownerPtr == IntPtr.Zero)
        {
            return 0;
        }

        try
        {
            // This checkpoint uses one fixed script type.
            Type scriptType = typeof(PlayerController);

            ConstructorInfo? constructor = scriptType
                .GetConstructors(
                    BindingFlags.Public
                    | BindingFlags.NonPublic
                    | BindingFlags.Instance
                )
                .FirstOrDefault(
                    candidate => candidate.GetParameters().Length == 0
                );

            if (constructor == null)
            {
                return 0;
            }

            // 1) Allocate the managed object without running constructors.
            var instance = (MingObject)RuntimeHelpers.GetUninitializedObject(scriptType);

            // 2) Give it the already-existing native owner.
            instance.NativePtr = ownerPtr;

            // 3) Run the constructor chain.
            _ = constructor.Invoke(instance, Array.Empty<object?>());

            s_lastScriptInstance = new WeakReference<PlayerController>((PlayerController)instance);

            return 1;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    private static int ValidateManagedScriptInstance(IntPtr handlePtr, IntPtr expectedOwner)
    {
        try
        {
            if (handlePtr == IntPtr.Zero || expectedOwner == IntPtr.Zero)
            {
                return 0;
            }

            // 1) Drop ordinary unreachable managed objects.
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            // 2) Resolve the target only after collection.
            GCHandle handle = GCHandle.FromIntPtr(handlePtr);

            if (handle.Target is not PlayerController player)
            {
                return 0;
            }

            // 3) Confirm this is still the wrapper of the original native owner.
            return player.ValidateNativeOwner(expectedOwner) ? 1 : 0;
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine(exception);
            return 0;
        }
    }

    [UnmanagedCallersOnly]
    private static unsafe void CollectAndGetManagedScriptState(
        int* allocated,
        int* disposed,
        int* freed,
        int* targetAlive
    )
    {
        GC.Collect();
        GC.WaitForPendingFinalizers();
        GC.Collect();

        *allocated = s_scriptHandleAllocated;
        *disposed = s_scriptHandleDisposed;
        *freed = s_scriptHandleFreed;
        *targetAlive = s_lastScriptInstance?.TryGetTarget(out _) == true ? 1 : 0;
    }

    #endregion

    // Store the native function pointer table passed from the engine.
    // e.g. NativeFuncs.Initialize(callbacksPtr, sizeof(NativeCallbacks))
    public static void Initialize(
        IntPtr nativeCallbacks,
        int nativeCallbackSize,
        IntPtr managedCallbacks,
        int managedCallbackSize
    )
    {
        if (
            nativeCallbackSize != sizeof(NativeCallbacks)
            || managedCallbackSize != sizeof(ManagedCallbacks)
        )
        {
            throw new InvalidOperationException("Callbacks size mismatch.");
        }

        s_callbacks = *(NativeCallbacks*)nativeCallbacks;
        *(ManagedCallbacks*)managedCallbacks = new ManagedCallbacks
        {
            Ping = &Ping,
            CreateTestGCHandle = &CreateTestGCHandle,
            InvokeTestGCHandle = &InvokeTestGCHandle,
            FreeGCHandle = &FreeGCHandle,
            CollectAndGetState = &CollectAndGetState,
            CreateManagedScriptInstance = &CreateManagedScriptInstance,
            ValidateManagedScriptInstance = &ValidateManagedScriptInstance,
            CollectAndGetManagedScriptState = &CollectAndGetManagedScriptState,
        };
    }
}
