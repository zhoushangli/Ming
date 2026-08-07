using System.Diagnostics;

namespace MingSharp
{
	public partial class PackedSceneLoader : ResourceFormatLoader
	{
		internal PackedSceneLoader(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
