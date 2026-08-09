using System.Diagnostics;

namespace Ming;

public partial class DirectionalLight3D : Light3D
{
	internal DirectionalLight3D(nint nativeHandle) : base(nativeHandle)
	{
	}

}
