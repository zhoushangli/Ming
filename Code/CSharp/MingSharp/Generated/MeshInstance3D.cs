using System.Diagnostics;

namespace Ming;

public partial class MeshInstance3D : VisualInstance3D
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetMeshResourceMethodBind = NativeFuncs.GetMethodBind("MeshInstance3D", "SetMeshResource");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetMeshResourceMethodBind = NativeFuncs.GetMethodBind("MeshInstance3D", "GetMeshResource");

	internal MeshInstance3D(nint nativeHandle) : base(nativeHandle)
	{
	}

}
