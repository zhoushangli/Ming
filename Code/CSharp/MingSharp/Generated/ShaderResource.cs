using System.Diagnostics;

namespace Ming;

public partial class ShaderResource : Resource
{
	internal ShaderResource(nint nativeHandle) : base(nativeHandle)
	{
	}

}
