using System.Diagnostics;

namespace MingSharp
{
	public partial class ShaderResource : Resource
	{
		internal ShaderResource(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
