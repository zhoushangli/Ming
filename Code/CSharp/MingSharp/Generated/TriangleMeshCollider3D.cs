using System.Diagnostics;

namespace Ming;

public partial class TriangleMeshCollider3D : Collider3D
{
	internal TriangleMeshCollider3D(nint nativeHandle) : base(nativeHandle)
	{
	}

}
