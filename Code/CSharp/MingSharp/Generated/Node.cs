using System.Diagnostics;

namespace MingSharp
{
	public partial class Node : MingObject
	{
		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetNameMethodBind = NativeCalls.GetMethodBind("Node", "SetName");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetNameMethodBind = NativeCalls.GetMethodBind("Node", "GetName");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetSerializableMethodBind = NativeCalls.GetMethodBind("Node", "SetSerializable");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetSerializableMethodBind = NativeCalls.GetMethodBind("Node", "GetSerializable");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetReadyMethodBind = NativeCalls.GetMethodBind("Node", "SetReady");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetReadyMethodBind = NativeCalls.GetMethodBind("Node", "GetReady");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetProcessMethodBind = NativeCalls.GetMethodBind("Node", "SetProcess");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetProcessMethodBind = NativeCalls.GetMethodBind("Node", "GetProcess");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr SetScriptMethodBind = NativeCalls.GetMethodBind("Node", "SetScript");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr GetScriptMethodBind = NativeCalls.GetMethodBind("Node", "GetScript");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr AddNodeMethodBind = NativeCalls.GetMethodBind("Node", "AddNode");

		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr DeleteNodeMethodBind = NativeCalls.GetMethodBind("Node", "DeleteNode");

		internal Node(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
