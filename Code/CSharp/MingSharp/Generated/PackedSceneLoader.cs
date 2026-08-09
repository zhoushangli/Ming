using System.Diagnostics;

namespace Ming;

public partial class PackedSceneLoader : ResourceFormatLoader
{
	internal PackedSceneLoader(nint nativeHandle) : base(nativeHandle)
	{
	}

}
