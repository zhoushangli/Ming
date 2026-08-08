using System.Diagnostics;

namespace MingSharp;

public partial class ProjectSettingsLoader : ResourceFormatLoader
{
	internal ProjectSettingsLoader(nint nativeHandle) : base(nativeHandle)
	{
	}

}
