using System.Diagnostics;

namespace MingSharp
{
	public partial class MeshInstance3D : VisualInstance3D
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetMeshResourceMethodBind = NativeCalls.GetMethodBind("MeshInstance3D", "SetMeshResource");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetMeshResourceMethodBind = NativeCalls.GetMethodBind("MeshInstance3D", "GetMeshResource");

		internal MeshInstance3D(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
