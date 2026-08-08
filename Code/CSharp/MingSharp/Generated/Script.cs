using System.Diagnostics;

namespace MingSharp;

public partial class Script : Resource
{
	internal Script(nint nativeHandle) : base(nativeHandle)
	{
	}

}
