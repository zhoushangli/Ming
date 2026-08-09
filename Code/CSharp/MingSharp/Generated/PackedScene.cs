using System.Diagnostics;

namespace Ming;

public partial class PackedScene : Resource
{
	internal PackedScene(nint nativeHandle) : base(nativeHandle)
	{
	}

}
