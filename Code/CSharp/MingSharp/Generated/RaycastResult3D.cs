using System.Diagnostics;

namespace MingSharp;

public partial class RaycastResult3D : RefCounted
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetDidImpactMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "SetDidImpact");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetDidImpactMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "GetDidImpact");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetImpactDistanceMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "SetImpactDistance");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetImpactDistanceMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "GetImpactDistance");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetImpactPositionMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "SetImpactPosition");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetImpactPositionMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "GetImpactPosition");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetImpactNormalMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "SetImpactNormal");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetImpactNormalMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "GetImpactNormal");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetRayStartPositionMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "SetRayStartPosition");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetRayStartPositionMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "GetRayStartPosition");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetRayForwardNormalMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "SetRayForwardNormal");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetRayForwardNormalMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "GetRayForwardNormal");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetRayMaxLengthMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "SetRayMaxLength");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetRayMaxLengthMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "GetRayMaxLength");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetOwnerMethodBind = NativeFuncs.GetMethodBind("RaycastResult3D", "GetOwner");

	internal RaycastResult3D(nint nativeHandle) : base(nativeHandle)
	{
	}

	public void SetDidImpact(bool arg1)
	{
		NativeCalls.MingCall_Void_Bool(SetDidImpactMethodBind, GetPtr(this), arg1.ToMingBool());
	}

	public bool GetDidImpact()
	{
		return NativeCalls.MingCall_Bool(GetDidImpactMethodBind, GetPtr(this)).ToBool();
	}

	public void SetImpactDistance(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetImpactDistanceMethodBind, GetPtr(this), arg1);
	}

	public float GetImpactDistance()
	{
		return NativeCalls.MingCall_Float(GetImpactDistanceMethodBind, GetPtr(this));
	}

	public void SetRayMaxLength(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetRayMaxLengthMethodBind, GetPtr(this), arg1);
	}

	public float GetRayMaxLength()
	{
		return NativeCalls.MingCall_Float(GetRayMaxLengthMethodBind, GetPtr(this));
	}

	public int GetOwner()
	{
		return NativeCalls.MingCall_Int(GetOwnerMethodBind, GetPtr(this));
	}

}
