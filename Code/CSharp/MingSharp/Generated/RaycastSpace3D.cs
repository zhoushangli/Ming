using System.Diagnostics;

namespace MingSharp;

public partial class RaycastSpace3D : MingObject
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr IntersectRayMethodBind = NativeFuncs.GetMethodBind("RaycastSpace3D", "IntersectRay");

	internal RaycastSpace3D(nint nativeHandle) : base(nativeHandle)
	{
	}

}
