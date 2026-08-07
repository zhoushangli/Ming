using System.Diagnostics;

namespace MingSharp
{
	public partial class DirectionalLight3D : Light3D
	{
		internal DirectionalLight3D(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
