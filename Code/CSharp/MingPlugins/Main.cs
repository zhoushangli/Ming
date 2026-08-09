using System.Runtime.InteropServices;
using System.Text;

using Ming;

namespace MingPlugins
{
	public static class Main
	{
		private static unsafe int Log(string message)
		{
			byte[] utf8Bytes = Encoding.UTF8.GetBytes(message);

			fixed (byte* text = utf8Bytes)
			{
				return NativeFuncs.LogUtf8(
					text,
					utf8Bytes.Length);
			}
		}

		[UnmanagedCallersOnly]
		private static unsafe int Initialize(
			void* nativeCallbacks,
			int nativeCallbacksSize)
		{
			try
			{
				if (nativeCallbacks == null)
				{
					return -1;
				}

				NativeFuncs.Initialize((IntPtr)nativeCallbacks, nativeCallbacksSize);

				Node node = new Node();
				string isReady = node.GetProcess().ToString();
				Log(isReady);
				node.SetProcess(true);
				isReady = node.GetProcess().ToString();
				Log(isReady);

				return 0;
			}
			catch (Exception exception)
			{
				Console.Error.WriteLine(exception);
				return -5;
			}
		}

		[UnmanagedCallersOnly]
		private static unsafe int Shutdown()
		{
			try
			{
				return 0;
			}
			catch (Exception)
			{
				return -1;
			}
		}
	}
}
