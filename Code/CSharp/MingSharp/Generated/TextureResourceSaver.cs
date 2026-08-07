using System.Diagnostics;

namespace MingSharp
{
	public partial class TextureResourceSaver : ResourceFormatSaver
	{
		internal TextureResourceSaver(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
