using System.Diagnostics;

namespace MingSharp
{
	public partial class ResourceFormatSaver : RefCounted
	{
		internal ResourceFormatSaver(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
