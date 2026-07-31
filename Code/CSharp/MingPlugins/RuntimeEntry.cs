using System.Runtime.InteropServices;

namespace MingPlugins
{
	public static class RuntimeEntry
	{
		[UnmanagedCallersOnly]
		public static void Initialize()
		{
			// Initialization logic here
		}

		[UnmanagedCallersOnly]
		public static void Shutdown()
		{
		}
	}
}
