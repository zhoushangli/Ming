using System.Diagnostics;

namespace MingSharp
{
	public partial class Image : RefCounted
	{
		internal Image(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
