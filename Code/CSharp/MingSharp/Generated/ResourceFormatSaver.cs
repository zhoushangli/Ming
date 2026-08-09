using System.Diagnostics;

namespace Ming;

public partial class ResourceFormatSaver : RefCounted
{
	internal ResourceFormatSaver(nint nativeHandle) : base(nativeHandle)
	{
	}

}
