using System.Diagnostics;

namespace Ming;

public partial class Camera3D : Node3D
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetNearClipMethodBind = NativeFuncs.GetMethodBind("Camera3D", "SetNearClip");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetNearClipMethodBind = NativeFuncs.GetMethodBind("Camera3D", "GetNearClip");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetFarClipMethodBind = NativeFuncs.GetMethodBind("Camera3D", "SetFarClip");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetFarClipMethodBind = NativeFuncs.GetMethodBind("Camera3D", "GetFarClip");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetFovDegreesMethodBind = NativeFuncs.GetMethodBind("Camera3D", "SetFovDegrees");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetFovDegreesMethodBind = NativeFuncs.GetMethodBind("Camera3D", "GetFovDegrees");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetSizeMethodBind = NativeFuncs.GetMethodBind("Camera3D", "SetSize");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetSizeMethodBind = NativeFuncs.GetMethodBind("Camera3D", "GetSize");

	internal Camera3D(nint nativeHandle) : base(nativeHandle)
	{
	}

	public void SetNearClip(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetNearClipMethodBind, GetPtr(this), arg1);
	}

	public float GetNearClip()
	{
		return NativeCalls.MingCall_Float(GetNearClipMethodBind, GetPtr(this));
	}

	public void SetFarClip(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetFarClipMethodBind, GetPtr(this), arg1);
	}

	public float GetFarClip()
	{
		return NativeCalls.MingCall_Float(GetFarClipMethodBind, GetPtr(this));
	}

	public void SetFovDegrees(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetFovDegreesMethodBind, GetPtr(this), arg1);
	}

	public float GetFovDegrees()
	{
		return NativeCalls.MingCall_Float(GetFovDegreesMethodBind, GetPtr(this));
	}

	public void SetSize(float arg1)
	{
		NativeCalls.MingCall_Void_Float(SetSizeMethodBind, GetPtr(this), arg1);
	}

	public float GetSize()
	{
		return NativeCalls.MingCall_Float(GetSizeMethodBind, GetPtr(this));
	}

}
