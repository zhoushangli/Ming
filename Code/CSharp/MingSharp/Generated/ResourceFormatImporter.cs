using System.Diagnostics;

namespace MingSharp
{
	public partial class ResourceFormatImporter : RefCounted
	{
		internal ResourceFormatImporter(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
