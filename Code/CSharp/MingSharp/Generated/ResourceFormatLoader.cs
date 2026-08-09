using System.Diagnostics;

namespace Ming;

public partial class ResourceFormatLoader : RefCounted
{
	internal ResourceFormatLoader(nint nativeHandle) : base(nativeHandle)
	{
	}

}
