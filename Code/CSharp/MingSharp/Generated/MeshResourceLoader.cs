using System.Diagnostics;

namespace MingSharp;

public partial class MeshResourceLoader : ResourceFormatLoader
{
	internal MeshResourceLoader(nint nativeHandle) : base(nativeHandle)
	{
	}

}
