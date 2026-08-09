using System.Diagnostics;

namespace Ming;

public partial class Object : MingObject
{
	internal Object(nint nativeHandle) : base(nativeHandle)
	{
	}

}
