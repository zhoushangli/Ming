using System.Diagnostics;

namespace MingSharp;

public partial class TriangleMeshCollider3D : Collider3D
{
	internal TriangleMeshCollider3D(nint nativeHandle) : base(nativeHandle)
	{
	}

}
