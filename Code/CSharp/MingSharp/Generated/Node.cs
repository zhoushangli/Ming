using System.Diagnostics;

namespace MingSharp;

public partial class Node : MingObject
{
	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetNameMethodBind = NativeFuncs.GetMethodBind("Node", "SetName");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetNameMethodBind = NativeFuncs.GetMethodBind("Node", "GetName");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetSerializableMethodBind = NativeFuncs.GetMethodBind("Node", "SetSerializable");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetSerializableMethodBind = NativeFuncs.GetMethodBind("Node", "GetSerializable");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetReadyMethodBind = NativeFuncs.GetMethodBind("Node", "SetReady");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetReadyMethodBind = NativeFuncs.GetMethodBind("Node", "GetReady");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetProcessMethodBind = NativeFuncs.GetMethodBind("Node", "SetProcess");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetProcessMethodBind = NativeFuncs.GetMethodBind("Node", "GetProcess");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr SetScriptMethodBind = NativeFuncs.GetMethodBind("Node", "SetScript");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr GetScriptMethodBind = NativeFuncs.GetMethodBind("Node", "GetScript");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr AddNodeMethodBind = NativeFuncs.GetMethodBind("Node", "AddNode");

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr DeleteNodeMethodBind = NativeFuncs.GetMethodBind("Node", "DeleteNode");

	internal Node(nint nativeHandle) : base(nativeHandle)
	{
	}

	public void SetSerializable(bool arg1)
	{
		NativeCalls.MingCall_Void_Bool(SetSerializableMethodBind, GetPtr(this), arg1.ToMingBool());
	}

	public bool GetSerializable()
	{
		return NativeCalls.MingCall_Bool(GetSerializableMethodBind, GetPtr(this)).ToBool();
	}

	public void SetReady(bool arg1)
	{
		NativeCalls.MingCall_Void_Bool(SetReadyMethodBind, GetPtr(this), arg1.ToMingBool());
	}

	public bool GetReady()
	{
		return NativeCalls.MingCall_Bool(GetReadyMethodBind, GetPtr(this)).ToBool();
	}

	public void SetProcess(bool arg1)
	{
		NativeCalls.MingCall_Void_Bool(SetProcessMethodBind, GetPtr(this), arg1.ToMingBool());
	}

	public bool GetProcess()
	{
		return NativeCalls.MingCall_Bool(GetProcessMethodBind, GetPtr(this)).ToBool();
	}

	public void DeleteNode()
	{
		NativeCalls.MingCall_Void(DeleteNodeMethodBind, GetPtr(this));
	}

}
