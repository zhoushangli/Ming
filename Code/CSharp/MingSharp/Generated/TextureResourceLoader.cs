using System.Diagnostics;

namespace MingSharp;

public partial class TextureResourceLoader : ResourceFormatLoader
{
	internal TextureResourceLoader(nint nativeHandle) : base(nativeHandle)
	{
	}

}
