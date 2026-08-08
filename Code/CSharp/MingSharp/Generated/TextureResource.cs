using System.Diagnostics;

namespace MingSharp;

public partial class TextureResource : Resource
{
	internal TextureResource(nint nativeHandle) : base(nativeHandle)
	{
	}

}
