using System.Runtime.CompilerServices;

namespace Ming;

public unsafe ref struct NativeVariantPtrArgs
{

    public int Count
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get => argc;
    }

    public ref ming_variant this[int index]
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get => ref *args[index];
    }

    private ming_variant** args;
    private int argc;

    internal NativeVariantPtrArgs(ming_variant** args, int argc)
    {
        this.args = args;
        this.argc = argc;
    }
}