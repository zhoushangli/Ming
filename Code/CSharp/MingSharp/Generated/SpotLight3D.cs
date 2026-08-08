using System.Diagnostics;

namespace MingSharp;

public partial class SpotLight3D : Light3D
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetRangeMethodBind = NativeFuncs.GetMethodBind("SpotLight3D", "SetRange");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetRangeMethodBind = NativeFuncs.GetMethodBind("SpotLight3D", "GetRange");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetAttenuationMethodBind = NativeFuncs.GetMethodBind("SpotLight3D", "SetAttenuation");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetAttenuationMethodBind = NativeFuncs.GetMethodBind("SpotLight3D", "GetAttenuation");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetSpotAngleMethodBind = NativeFuncs.GetMethodBind("SpotLight3D", "SetSpotAngle");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetSpotAngleMethodBind = NativeFuncs.GetMethodBind("SpotLight3D", "GetSpotAngle");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetSpotAttenuationMethodBind = NativeFuncs.GetMethodBind("SpotLight3D", "SetSpotAttenuation");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetSpotAttenuationMethodBind = NativeFuncs.GetMethodBind("SpotLight3D", "GetSpotAttenuation");

	internal SpotLight3D(nint nativeHandle) : base(nativeHandle)
	{
	}

	public void SetRange(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetRangeMethodBind, GetPtr(this), arg1);
	}

	public float GetRange()
	{
		return NativeCalls.MingCall_Float(GetRangeMethodBind, GetPtr(this));
	}

	public void SetAttenuation(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetAttenuationMethodBind, GetPtr(this), arg1);
	}

	public float GetAttenuation()
	{
		return NativeCalls.MingCall_Float(GetAttenuationMethodBind, GetPtr(this));
	}

	public void SetSpotAngle(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetSpotAngleMethodBind, GetPtr(this), arg1);
	}

	public float GetSpotAngle()
	{
		return NativeCalls.MingCall_Float(GetSpotAngleMethodBind, GetPtr(this));
	}

	public void SetSpotAttenuation(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetSpotAttenuationMethodBind, GetPtr(this), arg1);
	}

	public float GetSpotAttenuation()
	{
		return NativeCalls.MingCall_Float(GetSpotAttenuationMethodBind, GetPtr(this));
	}

}
