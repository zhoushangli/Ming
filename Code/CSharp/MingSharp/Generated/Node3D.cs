using System.Diagnostics;

namespace MingSharp
{
	public partial class Node3D : Node
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetLocalTransformMethodBind = NativeCalls.GetMethodBind("Node3D", "SetLocalTransform");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetLocalTransformMethodBind = NativeCalls.GetMethodBind("Node3D", "GetLocalTransform");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetLocalPositionMethodBind = NativeCalls.GetMethodBind("Node3D", "SetLocalPosition");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetLocalPositionMethodBind = NativeCalls.GetMethodBind("Node3D", "GetLocalPosition");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetLocalOrientationMethodBind = NativeCalls.GetMethodBind("Node3D", "SetLocalOrientation");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetLocalOrientationMethodBind = NativeCalls.GetMethodBind("Node3D", "GetLocalOrientation");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetLocalScaleMethodBind = NativeCalls.GetMethodBind("Node3D", "SetLocalScale");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetLocalScaleMethodBind = NativeCalls.GetMethodBind("Node3D", "GetLocalScale");

		internal Node3D(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
