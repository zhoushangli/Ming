using System.Diagnostics;

namespace Ming;

public partial class PackedSceneSaver : ResourceFormatSaver
{
	internal PackedSceneSaver(nint nativeHandle) : base(nativeHandle)
	{
	}

}
