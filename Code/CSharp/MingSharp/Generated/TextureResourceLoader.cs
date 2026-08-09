using System.Diagnostics;

namespace Ming;

public partial class TextureResourceLoader : ResourceFormatLoader
{
	internal TextureResourceLoader(nint nativeHandle) : base(nativeHandle)
	{
	}

}
