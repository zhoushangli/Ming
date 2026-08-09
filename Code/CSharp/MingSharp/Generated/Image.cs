using System.Diagnostics;

namespace Ming;

public partial class Image : RefCounted
{
	internal Image(nint nativeHandle) : base(nativeHandle)
	{
	}

}
