using System.Diagnostics;

namespace Ming;

public partial class ShaderResourceLoader : ResourceFormatLoader
{
	internal ShaderResourceLoader(nint nativeHandle) : base(nativeHandle)
	{
	}

}
