using System.Diagnostics;

namespace Ming;

public partial class ResourceFormatImporter : RefCounted
{
	internal ResourceFormatImporter(nint nativeHandle) : base(nativeHandle)
	{
	}

}
