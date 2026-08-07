using System.Diagnostics;

namespace MingSharp
{
	public partial class Light3D : Node3D
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetIntensityMethodBind = NativeCalls.GetMethodBind("Light3D", "SetIntensity");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetIntensityMethodBind = NativeCalls.GetMethodBind("Light3D", "GetIntensity");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetColorMethodBind = NativeCalls.GetMethodBind("Light3D", "SetColor");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetColorMethodBind = NativeCalls.GetMethodBind("Light3D", "GetColor");

		internal Light3D(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
