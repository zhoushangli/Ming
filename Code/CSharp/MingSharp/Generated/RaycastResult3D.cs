using System.Diagnostics;

namespace MingSharp
{
	public partial class RaycastResult3D : RefCounted
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetDidImpactMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "SetDidImpact");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetDidImpactMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "GetDidImpact");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetImpactDistanceMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "SetImpactDistance");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetImpactDistanceMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "GetImpactDistance");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetImpactPositionMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "SetImpactPosition");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetImpactPositionMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "GetImpactPosition");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetImpactNormalMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "SetImpactNormal");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetImpactNormalMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "GetImpactNormal");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetRayStartPositionMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "SetRayStartPosition");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetRayStartPositionMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "GetRayStartPosition");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetRayForwardNormalMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "SetRayForwardNormal");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetRayForwardNormalMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "GetRayForwardNormal");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetRayMaxLengthMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "SetRayMaxLength");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetRayMaxLengthMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "GetRayMaxLength");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetOwnerMethodBind = NativeCalls.GetMethodBind("RaycastResult3D", "GetOwner");

		internal RaycastResult3D(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
