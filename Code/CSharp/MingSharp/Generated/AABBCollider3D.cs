using System.Diagnostics;

namespace Ming;

public partial class AABBCollider3D : Collider3D
{
	internal AABBCollider3D(nint nativeHandle) : base(nativeHandle)
	{
	}

}
