using System.Diagnostics;

namespace MingSharp
{
	public partial class Camera3D : Node3D
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetNearClipMethodBind = NativeCalls.GetMethodBind("Camera3D", "SetNearClip");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetNearClipMethodBind = NativeCalls.GetMethodBind("Camera3D", "GetNearClip");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetFarClipMethodBind = NativeCalls.GetMethodBind("Camera3D", "SetFarClip");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetFarClipMethodBind = NativeCalls.GetMethodBind("Camera3D", "GetFarClip");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetFovDegreesMethodBind = NativeCalls.GetMethodBind("Camera3D", "SetFovDegrees");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetFovDegreesMethodBind = NativeCalls.GetMethodBind("Camera3D", "GetFovDegrees");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetSizeMethodBind = NativeCalls.GetMethodBind("Camera3D", "SetSize");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetSizeMethodBind = NativeCalls.GetMethodBind("Camera3D", "GetSize");

		internal Camera3D(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
