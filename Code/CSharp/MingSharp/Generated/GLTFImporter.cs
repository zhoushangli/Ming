using System.Diagnostics;

namespace MingSharp;

public partial class GLTFImporter : ResourceFormatImporter
{
	internal GLTFImporter(nint nativeHandle) : base(nativeHandle)
	{
	}

}
