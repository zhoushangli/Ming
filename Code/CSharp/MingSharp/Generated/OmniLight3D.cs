using System.Diagnostics;

namespace MingSharp
{
	public partial class OmniLight3D : Light3D
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetRangeMethodBind = NativeCalls.GetMethodBind("OmniLight3D", "SetRange");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetRangeMethodBind = NativeCalls.GetMethodBind("OmniLight3D", "GetRange");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetAttenuationMethodBind = NativeCalls.GetMethodBind("OmniLight3D", "SetAttenuation");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetAttenuationMethodBind = NativeCalls.GetMethodBind("OmniLight3D", "GetAttenuation");

		internal OmniLight3D(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
