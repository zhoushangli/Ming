using System.Diagnostics;

namespace MingSharp;

public partial class MeshResourceSaver : ResourceFormatSaver
{
	internal MeshResourceSaver(nint nativeHandle) : base(nativeHandle)
	{
	}

}
