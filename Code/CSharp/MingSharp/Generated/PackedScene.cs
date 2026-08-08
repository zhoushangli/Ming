using System.Diagnostics;

namespace MingSharp;

public partial class PackedScene : Resource
{
	internal PackedScene(nint nativeHandle) : base(nativeHandle)
	{
	}

}
