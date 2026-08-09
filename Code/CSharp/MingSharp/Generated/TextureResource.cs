using System.Diagnostics;

namespace Ming;

public partial class TextureResource : Resource
{
	internal TextureResource(nint nativeHandle) : base(nativeHandle)
	{
	}

}
