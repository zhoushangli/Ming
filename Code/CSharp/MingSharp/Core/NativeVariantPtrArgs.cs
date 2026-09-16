using System.Runtime.CompilerServices;

namespace Ming;

public unsafe ref struct NativeVariantPtrArgs
{
    private ming_variant** args;
    private int argc;

    internal NativeVariantPtrArgs(ming_variant** args, int argc)
    {
        this.args = args;
        this.argc = argc;
    }

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
}