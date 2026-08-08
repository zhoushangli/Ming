using System.Diagnostics;

namespace MingSharp;

public partial class AABBCollider3D : Collider3D
{
	internal AABBCollider3D(nint nativeHandle) : base(nativeHandle)
	{
	}

}
