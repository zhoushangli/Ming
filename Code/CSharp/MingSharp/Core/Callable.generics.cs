namespace Ming;

// Retain typed delegates; only a direct MingObject Target supplies receiver lifetime, and reload invalidates all delegates.
// e.g. Save an Action and wrap it again to disconnect a lambda; captured MingObjects require manual lifetime management.
public sealed partial class Callable
{
    public static unsafe Callable From(Action callback)
    {
        ArgumentNullException.ThrowIfNull(callback);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 0, out error))
            {
                return;
            }

            ((Action)value)();
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0>(Action<T0> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            ((Action<T0>)value)(arg0);
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1>(Action<T0, T1> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            ((Action<T0, T1>)value)(arg0, arg1);
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2>(Action<T0, T1, T2> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            ((Action<T0, T1, T2>)value)(arg0, arg1, arg2);
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3>(Action<T0, T1, T2, T3> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 4, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            ((Action<T0, T1, T2, T3>)value)(arg0, arg1, arg2, arg3);
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, T4>(Action<T0, T1, T2, T3, T4> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<T4>(true);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 5, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T4>(args[4], 4, VariantUtils.GetVariantType<T4>(true),
                out T4 arg4, out error))
            {
                return;
            }

            ((Action<T0, T1, T2, T3, T4>)value)(arg0, arg1, arg2, arg3, arg4);
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, T4, T5>(Action<T0, T1, T2, T3, T4, T5> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<T4>(true);
        VariantUtils.GetVariantType<T5>(true);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 6, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T4>(args[4], 4, VariantUtils.GetVariantType<T4>(true),
                out T4 arg4, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T5>(args[5], 5, VariantUtils.GetVariantType<T5>(true),
                out T5 arg5, out error))
            {
                return;
            }

            ((Action<T0, T1, T2, T3, T4, T5>)value)(arg0, arg1, arg2, arg3, arg4, arg5);
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, T4, T5, T6>(Action<T0, T1, T2, T3, T4, T5, T6> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<T4>(true);
        VariantUtils.GetVariantType<T5>(true);
        VariantUtils.GetVariantType<T6>(true);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 7, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T4>(args[4], 4, VariantUtils.GetVariantType<T4>(true),
                out T4 arg4, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T5>(args[5], 5, VariantUtils.GetVariantType<T5>(true),
                out T5 arg5, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T6>(args[6], 6, VariantUtils.GetVariantType<T6>(true),
                out T6 arg6, out error))
            {
                return;
            }

            ((Action<T0, T1, T2, T3, T4, T5, T6>)value)(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, T4, T5, T6, T7>(Action<T0, T1, T2, T3, T4, T5, T6, T7> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<T4>(true);
        VariantUtils.GetVariantType<T5>(true);
        VariantUtils.GetVariantType<T6>(true);
        VariantUtils.GetVariantType<T7>(true);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 8, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T4>(args[4], 4, VariantUtils.GetVariantType<T4>(true),
                out T4 arg4, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T5>(args[5], 5, VariantUtils.GetVariantType<T5>(true),
                out T5 arg5, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T6>(args[6], 6, VariantUtils.GetVariantType<T6>(true),
                out T6 arg6, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T7>(args[7], 7, VariantUtils.GetVariantType<T7>(true),
                out T7 arg7, out error))
            {
                return;
            }

            ((Action<T0, T1, T2, T3, T4, T5, T6, T7>)value)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, T4, T5, T6, T7, T8>(Action<T0, T1, T2, T3, T4, T5, T6, T7, T8> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<T4>(true);
        VariantUtils.GetVariantType<T5>(true);
        VariantUtils.GetVariantType<T6>(true);
        VariantUtils.GetVariantType<T7>(true);
        VariantUtils.GetVariantType<T8>(true);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 9, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T4>(args[4], 4, VariantUtils.GetVariantType<T4>(true),
                out T4 arg4, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T5>(args[5], 5, VariantUtils.GetVariantType<T5>(true),
                out T5 arg5, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T6>(args[6], 6, VariantUtils.GetVariantType<T6>(true),
                out T6 arg6, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T7>(args[7], 7, VariantUtils.GetVariantType<T7>(true),
                out T7 arg7, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T8>(args[8], 8, VariantUtils.GetVariantType<T8>(true),
                out T8 arg8, out error))
            {
                return;
            }

            ((Action<T0, T1, T2, T3, T4, T5, T6, T7, T8>)value)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<TResult>(Func<TResult> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<TResult>(false);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 0, out error))
            {
                return;
            }

            ret = VariantUtils.CreateFrom(((Func<TResult>)value)());
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, TResult>(Func<T0, TResult> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<TResult>(false);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            ret = VariantUtils.CreateFrom(((Func<T0, TResult>)value)(arg0));
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, TResult>(Func<T0, T1, TResult> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<TResult>(false);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            ret = VariantUtils.CreateFrom(((Func<T0, T1, TResult>)value)(arg0, arg1));
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, TResult>(Func<T0, T1, T2, TResult> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<TResult>(false);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            ret = VariantUtils.CreateFrom(((Func<T0, T1, T2, TResult>)value)(arg0, arg1, arg2));
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, TResult>(Func<T0, T1, T2, T3, TResult> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<TResult>(false);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 4, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            ret = VariantUtils.CreateFrom(((Func<T0, T1, T2, T3, TResult>)value)(arg0, arg1, arg2, arg3));
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, T4, TResult>(Func<T0, T1, T2, T3, T4, TResult> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<T4>(true);
        VariantUtils.GetVariantType<TResult>(false);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 5, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T4>(args[4], 4, VariantUtils.GetVariantType<T4>(true),
                out T4 arg4, out error))
            {
                return;
            }

            ret = VariantUtils.CreateFrom(((Func<T0, T1, T2, T3, T4, TResult>)value)(arg0, arg1, arg2, arg3, arg4));
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, T4, T5, TResult>(Func<T0, T1, T2, T3, T4, T5, TResult> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<T4>(true);
        VariantUtils.GetVariantType<T5>(true);
        VariantUtils.GetVariantType<TResult>(false);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 6, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T4>(args[4], 4, VariantUtils.GetVariantType<T4>(true),
                out T4 arg4, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T5>(args[5], 5, VariantUtils.GetVariantType<T5>(true),
                out T5 arg5, out error))
            {
                return;
            }

            ret = VariantUtils.CreateFrom(((Func<T0, T1, T2, T3, T4, T5, TResult>)value)(arg0, arg1, arg2, arg3, arg4, arg5));
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, T4, T5, T6, TResult>(Func<T0, T1, T2, T3, T4, T5, T6, TResult> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<T4>(true);
        VariantUtils.GetVariantType<T5>(true);
        VariantUtils.GetVariantType<T6>(true);
        VariantUtils.GetVariantType<TResult>(false);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 7, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T4>(args[4], 4, VariantUtils.GetVariantType<T4>(true),
                out T4 arg4, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T5>(args[5], 5, VariantUtils.GetVariantType<T5>(true),
                out T5 arg5, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T6>(args[6], 6, VariantUtils.GetVariantType<T6>(true),
                out T6 arg6, out error))
            {
                return;
            }

            ret = VariantUtils.CreateFrom(((Func<T0, T1, T2, T3, T4, T5, T6, TResult>)value)(arg0, arg1, arg2, arg3, arg4, arg5, arg6));
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, T4, T5, T6, T7, TResult>(Func<T0, T1, T2, T3, T4, T5, T6, T7, TResult> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<T4>(true);
        VariantUtils.GetVariantType<T5>(true);
        VariantUtils.GetVariantType<T6>(true);
        VariantUtils.GetVariantType<T7>(true);
        VariantUtils.GetVariantType<TResult>(false);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 8, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T4>(args[4], 4, VariantUtils.GetVariantType<T4>(true),
                out T4 arg4, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T5>(args[5], 5, VariantUtils.GetVariantType<T5>(true),
                out T5 arg5, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T6>(args[6], 6, VariantUtils.GetVariantType<T6>(true),
                out T6 arg6, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T7>(args[7], 7, VariantUtils.GetVariantType<T7>(true),
                out T7 arg7, out error))
            {
                return;
            }

            ret = VariantUtils.CreateFrom(((Func<T0, T1, T2, T3, T4, T5, T6, T7, TResult>)value)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7));
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    public static unsafe Callable From<T0, T1, T2, T3, T4, T5, T6, T7, T8, TResult>(Func<T0, T1, T2, T3, T4, T5, T6, T7, T8, TResult> callback)
    {
        ArgumentNullException.ThrowIfNull(callback);
        VariantUtils.GetVariantType<T0>(true);
        VariantUtils.GetVariantType<T1>(true);
        VariantUtils.GetVariantType<T2>(true);
        VariantUtils.GetVariantType<T3>(true);
        VariantUtils.GetVariantType<T4>(true);
        VariantUtils.GetVariantType<T5>(true);
        VariantUtils.GetVariantType<T6>(true);
        VariantUtils.GetVariantType<T7>(true);
        VariantUtils.GetVariantType<T8>(true);
        VariantUtils.GetVariantType<TResult>(false);

        static void Trampoline(Delegate value, NativeVariantPtrArgs args, out ming_variant ret, out ming_call_error error)
        {
            ret = default;
            if (!CheckArgumentCount(args.Count, 9, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T0>(args[0], 0, VariantUtils.GetVariantType<T0>(true),
                out T0 arg0, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T1>(args[1], 1, VariantUtils.GetVariantType<T1>(true),
                out T1 arg1, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T2>(args[2], 2, VariantUtils.GetVariantType<T2>(true),
                out T2 arg2, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T3>(args[3], 3, VariantUtils.GetVariantType<T3>(true),
                out T3 arg3, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T4>(args[4], 4, VariantUtils.GetVariantType<T4>(true),
                out T4 arg4, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T5>(args[5], 5, VariantUtils.GetVariantType<T5>(true),
                out T5 arg5, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T6>(args[6], 6, VariantUtils.GetVariantType<T6>(true),
                out T6 arg6, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T7>(args[7], 7, VariantUtils.GetVariantType<T7>(true),
                out T7 arg7, out error))
            {
                return;
            }

            if (!VariantUtils.TryConvertTo<T8>(args[8], 8, VariantUtils.GetVariantType<T8>(true),
                out T8 arg8, out error))
            {
                return;
            }

            ret = VariantUtils.CreateFrom(((Func<T0, T1, T2, T3, T4, T5, T6, T7, T8, TResult>)value)(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
        }

        return CreateFromDelegate(callback, &Trampoline);
    }

    private static bool CheckArgumentCount(int received, int expected, out ming_call_error error)
    {
        error = new ming_call_error();
        if (received != expected)
        {
            error.Error = received < expected ? CallErrorCode.TooFewArguments : CallErrorCode.TooManyArguments;
            error.ExpectedArgumentCount = expected;
            return false;
        }

        return true;
    }

    private static unsafe Callable CreateFromDelegate(Delegate callback,
        delegate* managed<Delegate, NativeVariantPtrArgs, out ming_variant, out ming_call_error, void> trampoline)
    {
        ScriptManagerBridge.EnsureNativeBindings();
        MingObject target = callback.Target as MingObject;
        IntPtr targetPtr = MingObject.GetPtr(target);
        IntPtr handle = ScriptManagerBridge.AllocDelegateGCHandle(callback);
        bool transferred = false;
        try
        {
            ming_callable value = NativeFuncs.CreateManagedCallable(targetPtr, handle, (IntPtr)trampoline);
            transferred = true;
            return TakeOwnershipOfNative(ref value);
        }
        finally
        {
            if (!transferred)
            {
                ScriptManagerBridge.ReleaseDelegateGCHandleCore(handle);
            }

            GC.KeepAlive(callback);
        }
    }
}
