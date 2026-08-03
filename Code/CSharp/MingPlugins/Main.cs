using System.Runtime.InteropServices;
using System.Text;

using MingSharp;

namespace MingPlugins
{
	public static class Main
	{
		private static unsafe int Log(string message)
		{
			byte[] utf8Bytes = Encoding.UTF8.GetBytes(message);

			fixed (byte* text = utf8Bytes)
			{
				return NativeCalls.LogUtf8(
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

				NativeCalls.Initialize((IntPtr)nativeCallbacks, nativeCallbacksSize);

				MingObject node = MingObject.Create("Node");
				string className = node.GetClassName();
				int logResult = Log(className);

				if (logResult != 0)
				{
					return -4;
				}

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
