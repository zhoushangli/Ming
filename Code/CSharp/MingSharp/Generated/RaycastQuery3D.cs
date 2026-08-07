using System.Diagnostics;

namespace MingSharp
{
	public partial class RaycastQuery3D : RefCounted
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetStartMethodBind = NativeCalls.GetMethodBind("RaycastQuery3D", "SetStart");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetStartMethodBind = NativeCalls.GetMethodBind("RaycastQuery3D", "GetStart");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetDirectionMethodBind = NativeCalls.GetMethodBind("RaycastQuery3D", "SetDirection");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetDirectionMethodBind = NativeCalls.GetMethodBind("RaycastQuery3D", "GetDirection");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetMaxDistanceMethodBind = NativeCalls.GetMethodBind("RaycastQuery3D", "SetMaxDistance");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetMaxDistanceMethodBind = NativeCalls.GetMethodBind("RaycastQuery3D", "GetMaxDistance");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetExcludeMethodBind = NativeCalls.GetMethodBind("RaycastQuery3D", "SetExclude");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetExcludeMethodBind = NativeCalls.GetMethodBind("RaycastQuery3D", "GetExclude");

		internal RaycastQuery3D(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
