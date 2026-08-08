namespace MingSharp;

internal static unsafe class NativeCalls
{
	internal static unsafe MingBool MingCall_Bool(IntPtr methodBind, IntPtr objectPtr)
	{
		MingBool ret;

		NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, null, &ret);

		return ret;
	}

	internal static unsafe MingBool MingCall_Bool_Int(IntPtr methodBind, IntPtr objectPtr, int arg1)
	{
		MingBool ret;

		void** args = stackalloc void*[1] { &arg1 };

		NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, args, &ret);

		return ret;
	}

	internal static unsafe float MingCall_Float(IntPtr methodBind, IntPtr objectPtr)
	{
		float ret;

		NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, null, &ret);

		return ret;
	}

	internal static unsafe int MingCall_Int(IntPtr methodBind, IntPtr objectPtr)
	{
		int ret;

		NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, null, &ret);

		return ret;
	}

	internal static unsafe void MingCall_Void(IntPtr methodBind, IntPtr objectPtr)
	{
		NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, null, null);
	}

	internal static unsafe void MingCall_Void_Bool(IntPtr methodBind, IntPtr objectPtr, MingBool arg1)
	{
		void** args = stackalloc void*[1] { &arg1 };

		NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, args, null);
	}

	internal static unsafe void MingCall_Void_Float(IntPtr methodBind, IntPtr objectPtr, float arg1)
	{
		void** args = stackalloc void*[1] { &arg1 };

		NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, args, null);
	}

	internal static unsafe void MingCall_Void_Int(IntPtr methodBind, IntPtr objectPtr, int arg1)
	{
		void** args = stackalloc void*[1] { &arg1 };

		NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, args, null);
	}

}
