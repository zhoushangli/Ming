using System.Diagnostics;

namespace MingSharp
{
	public partial class SpotLight3D : Light3D
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetRangeMethodBind = NativeCalls.GetMethodBind("SpotLight3D", "SetRange");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetRangeMethodBind = NativeCalls.GetMethodBind("SpotLight3D", "GetRange");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetAttenuationMethodBind = NativeCalls.GetMethodBind("SpotLight3D", "SetAttenuation");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetAttenuationMethodBind = NativeCalls.GetMethodBind("SpotLight3D", "GetAttenuation");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetSpotAngleMethodBind = NativeCalls.GetMethodBind("SpotLight3D", "SetSpotAngle");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetSpotAngleMethodBind = NativeCalls.GetMethodBind("SpotLight3D", "GetSpotAngle");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetSpotAttenuationMethodBind = NativeCalls.GetMethodBind("SpotLight3D", "SetSpotAttenuation");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetSpotAttenuationMethodBind = NativeCalls.GetMethodBind("SpotLight3D", "GetSpotAttenuation");

		internal SpotLight3D(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
