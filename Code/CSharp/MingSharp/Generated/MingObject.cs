namespace MingSharp;

public class MingObject
{
	internal IntPtr NativePtr;

	internal MingObject(IntPtr ptr)
	{
		NativePtr = ptr;
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

	// Create an engine object from the class database and return a C# wrapper of it.
	// e.g. MingObject.Create("Node")
	public static MingObject Create(string className)
	{
		return new MingObject(NativeFuncs.CreateObject(className));
	}

	// Return the engine class name of this object.
	// e.g. node.GetClassName() -> "Node"
	public string GetClassName()
	{
		return NativeFuncs.GetClassName(GetPtr(this));
	}
}
