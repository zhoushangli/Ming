using System.Diagnostics;

namespace MingSharp;

public partial class Object : MingObject
{
	internal Object(nint nativeHandle) : base(nativeHandle)
	{
	}

}
