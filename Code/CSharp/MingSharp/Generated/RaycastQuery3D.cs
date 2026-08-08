using System.Diagnostics;

namespace MingSharp;

public partial class RaycastQuery3D : RefCounted
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetStartMethodBind = NativeFuncs.GetMethodBind("RaycastQuery3D", "SetStart");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetStartMethodBind = NativeFuncs.GetMethodBind("RaycastQuery3D", "GetStart");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetDirectionMethodBind = NativeFuncs.GetMethodBind("RaycastQuery3D", "SetDirection");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetDirectionMethodBind = NativeFuncs.GetMethodBind("RaycastQuery3D", "GetDirection");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetMaxDistanceMethodBind = NativeFuncs.GetMethodBind("RaycastQuery3D", "SetMaxDistance");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetMaxDistanceMethodBind = NativeFuncs.GetMethodBind("RaycastQuery3D", "GetMaxDistance");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetExcludeMethodBind = NativeFuncs.GetMethodBind("RaycastQuery3D", "SetExclude");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetExcludeMethodBind = NativeFuncs.GetMethodBind("RaycastQuery3D", "GetExclude");

	internal RaycastQuery3D(nint nativeHandle) : base(nativeHandle)
	{
	}

	public void SetMaxDistance(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetMaxDistanceMethodBind, GetPtr(this), arg1);
	}

	public float GetMaxDistance()
	{
		return NativeCalls.MingCall_Float(GetMaxDistanceMethodBind, GetPtr(this));
	}

}
