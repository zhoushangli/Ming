using System.Diagnostics;

namespace MingSharp;

public partial class OmniLight3D : Light3D
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetRangeMethodBind = NativeFuncs.GetMethodBind("OmniLight3D", "SetRange");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetRangeMethodBind = NativeFuncs.GetMethodBind("OmniLight3D", "GetRange");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetAttenuationMethodBind = NativeFuncs.GetMethodBind("OmniLight3D", "SetAttenuation");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetAttenuationMethodBind = NativeFuncs.GetMethodBind("OmniLight3D", "GetAttenuation");

	internal OmniLight3D(nint nativeHandle) : base(nativeHandle)
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

}
