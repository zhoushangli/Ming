using System.Diagnostics;

namespace MingSharp
{
	public partial class ShaderResourceLoader : ResourceFormatLoader
	{
		internal ShaderResourceLoader(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
