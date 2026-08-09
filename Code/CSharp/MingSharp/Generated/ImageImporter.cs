using System.Diagnostics;

namespace Ming;

public partial class ImageImporter : ResourceFormatImporter
{
	internal ImageImporter(nint nativeHandle) : base(nativeHandle)
	{
	}

}
