using System.Diagnostics;

namespace MingSharp
{
	public partial class InputSystem : SystemBase
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr IsKeyDownMethodBind = NativeCalls.GetMethodBind("InputSystem", "IsKeyDown");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr WasKeyJustPressedMethodBind = NativeCalls.GetMethodBind("InputSystem", "WasKeyJustPressed");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr WasKeyJustReleasedMethodBind = NativeCalls.GetMethodBind("InputSystem", "WasKeyJustReleased");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetCursorModeMethodBind = NativeCalls.GetMethodBind("InputSystem", "SetCursorMode");

		internal InputSystem(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
