using System.Diagnostics;

namespace MingSharp;

public partial class Resource : RefCounted
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetPathMethodBind = NativeFuncs.GetMethodBind("Resource", "GetPath");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetPathMethodBind = NativeFuncs.GetMethodBind("Resource", "SetPath");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetNameMethodBind = NativeFuncs.GetMethodBind("Resource", "GetName");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetNameMethodBind = NativeFuncs.GetMethodBind("Resource", "SetName");

	internal Resource(nint nativeHandle) : base(nativeHandle)
	{
	}

}
