using System.Runtime.InteropServices;
using System.Text;

namespace MingPlugins
{
	public static class Main
	{
		[StructLayout(LayoutKind.Sequential)]
		private unsafe struct NativeCallbacks
		{
			public delegate* unmanaged<
				byte*,
				int,
				int> LogUtf8;
		}

		private static unsafe NativeCallbacks s_nativeCallbacks;

		private static unsafe int Log(string message)
		{
			byte[] utf8Bytes = Encoding.UTF8.GetBytes(message);

			fixed (byte* text = utf8Bytes)
			{
				return s_nativeCallbacks.LogUtf8(
					text,
					utf8Bytes.Length);
			}
		}


		[UnmanagedCallersOnly]
		private static unsafe int Initialize(
			NativeCallbacks* nativeCallbacks,
			int nativeCallbacksSize)
		{
			try
			{
				if (nativeCallbacks == null)
				{
					return -1;
				}

				if (nativeCallbacksSize != sizeof(NativeCallbacks))
				{
					return -2;
				}

				if (nativeCallbacks->LogUtf8 == null)
				{
					return -3;
				}

				s_nativeCallbacks = *nativeCallbacks;

				int logResult = Log(
					"Managed bridge initialized.");

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
			catch (Exception e)
			{
				return -1;
			}
		}
	}
}
