using System.Diagnostics;

namespace MingSharp;

public partial class Light3D : Node3D
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetIntensityMethodBind = NativeFuncs.GetMethodBind("Light3D", "SetIntensity");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetIntensityMethodBind = NativeFuncs.GetMethodBind("Light3D", "GetIntensity");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetColorMethodBind = NativeFuncs.GetMethodBind("Light3D", "SetColor");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetColorMethodBind = NativeFuncs.GetMethodBind("Light3D", "GetColor");

	internal Light3D(nint nativeHandle) : base(nativeHandle)
	{
	}

	public void SetIntensity(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetIntensityMethodBind, GetPtr(this), arg1);
	}

	public float GetIntensity()
	{
		return NativeCalls.MingCall_Float(GetIntensityMethodBind, GetPtr(this));
	}

}
