using System.Diagnostics;

namespace Ming;

public partial class ProjectSettingsSaver : ResourceFormatSaver
{
	internal ProjectSettingsSaver(nint nativeHandle) : base(nativeHandle)
	{
	}

}
