using System.Diagnostics;

namespace Ming;

public partial class TextureResourceSaver : ResourceFormatSaver
{
	internal TextureResourceSaver(nint nativeHandle) : base(nativeHandle)
	{
	}

}
