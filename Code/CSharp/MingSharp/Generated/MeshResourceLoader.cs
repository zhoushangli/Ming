using System.Diagnostics;

namespace Ming;

public partial class MeshResourceLoader : ResourceFormatLoader
{
	internal MeshResourceLoader(nint nativeHandle) : base(nativeHandle)
	{
	}

}
