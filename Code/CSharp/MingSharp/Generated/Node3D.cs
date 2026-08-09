using System.Diagnostics;

namespace Ming;

public partial class Node3D : Node
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetLocalTransformMethodBind = NativeFuncs.GetMethodBind("Node3D", "SetLocalTransform");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetLocalTransformMethodBind = NativeFuncs.GetMethodBind("Node3D", "GetLocalTransform");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetLocalPositionMethodBind = NativeFuncs.GetMethodBind("Node3D", "SetLocalPosition");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetLocalPositionMethodBind = NativeFuncs.GetMethodBind("Node3D", "GetLocalPosition");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetLocalOrientationMethodBind = NativeFuncs.GetMethodBind("Node3D", "SetLocalOrientation");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetLocalOrientationMethodBind = NativeFuncs.GetMethodBind("Node3D", "GetLocalOrientation");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetLocalScaleMethodBind = NativeFuncs.GetMethodBind("Node3D", "SetLocalScale");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetLocalScaleMethodBind = NativeFuncs.GetMethodBind("Node3D", "GetLocalScale");

	internal Node3D(nint nativeHandle) : base(nativeHandle)
	{
	}

}
