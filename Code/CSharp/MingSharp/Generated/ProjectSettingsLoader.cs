using System.Diagnostics;

namespace Ming;

public partial class ProjectSettingsLoader : ResourceFormatLoader
{
	internal ProjectSettingsLoader(nint nativeHandle) : base(nativeHandle)
	{
	}

}
