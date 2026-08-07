using System.Diagnostics;

namespace MingSharp
{
	public partial class ResourceFormatLoader : RefCounted
	{
		internal ResourceFormatLoader(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
