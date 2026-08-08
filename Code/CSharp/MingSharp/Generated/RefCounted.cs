using System.Diagnostics;

namespace MingSharp;

public partial class RefCounted : MingObject
{
	internal RefCounted(nint nativeHandle) : base(nativeHandle)
	{
	}

}
