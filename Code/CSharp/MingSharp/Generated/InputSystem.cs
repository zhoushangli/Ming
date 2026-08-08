using System.Diagnostics;

namespace MingSharp;

public partial class InputSystem : SystemBase
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr IsKeyDownMethodBind = NativeFuncs.GetMethodBind("InputSystem", "IsKeyDown");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr WasKeyJustPressedMethodBind = NativeFuncs.GetMethodBind("InputSystem", "WasKeyJustPressed");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr WasKeyJustReleasedMethodBind = NativeFuncs.GetMethodBind("InputSystem", "WasKeyJustReleased");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetCursorModeMethodBind = NativeFuncs.GetMethodBind("InputSystem", "SetCursorMode");

	internal InputSystem(nint nativeHandle) : base(nativeHandle)
	{
	}

	public bool IsKeyDown(int arg1)
	{
		return NativeCalls.MingCall_Bool_Int(IsKeyDownMethodBind, GetPtr(this), arg1).ToBool();
	}

	public bool WasKeyJustPressed(int arg1)
	{
		return NativeCalls.MingCall_Bool_Int(WasKeyJustPressedMethodBind, GetPtr(this), arg1).ToBool();
	}

	public bool WasKeyJustReleased(int arg1)
	{
		return NativeCalls.MingCall_Bool_Int(WasKeyJustReleasedMethodBind, GetPtr(this), arg1).ToBool();
	}

	public void SetCursorMode(int arg1)
	{
		NativeCalls.MingCall_Void_Int(SetCursorModeMethodBind, GetPtr(this), arg1);
	}

}
