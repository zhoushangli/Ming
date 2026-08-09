using System.Diagnostics;

namespace Ming;

public partial class ProjectSettings : Resource
{
	internal ProjectSettings(nint nativeHandle) : base(nativeHandle)
	{
	}

}
