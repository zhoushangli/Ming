using System.Diagnostics;

namespace Ming;

public partial class RefCounted : MingObject
{
	internal RefCounted(nint nativeHandle) : base(nativeHandle)
	{
	}

}
