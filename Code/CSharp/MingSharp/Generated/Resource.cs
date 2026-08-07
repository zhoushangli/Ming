using System.Diagnostics;

namespace MingSharp
{
	public partial class Resource : RefCounted
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetPathMethodBind = NativeCalls.GetMethodBind("Resource", "GetPath");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetPathMethodBind = NativeCalls.GetMethodBind("Resource", "SetPath");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetNameMethodBind = NativeCalls.GetMethodBind("Resource", "GetName");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetNameMethodBind = NativeCalls.GetMethodBind("Resource", "SetName");

		internal Resource(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
