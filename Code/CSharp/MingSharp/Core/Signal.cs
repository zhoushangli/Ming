namespace Ming;

// Identify a signal through its managed owner and name without owning native objects or subscriptions.
// e.g. new Signal(button, BaseButton.SignalName.Toggled).Connect(callback) uses the existing native connection table.
public readonly struct Signal : IEquatable<Signal>
{
    public MingObject Owner { get; }
    public string Name { get; }
    public bool IsValid => Owner != null && Owner.NativePtr != IntPtr.Zero && !string.IsNullOrEmpty(Name);

    public Signal(MingObject owner, string name)
    {
        ArgumentNullException.ThrowIfNull(owner);
        ArgumentException.ThrowIfNullOrEmpty(name);
        MingObject.GetPtr(owner);
        Owner = owner;
        Name = name;
    }

    public bool Connect(Callable callback)
    {
        return IsValid && Owner.Connect(Name, callback);
    }

    public bool Disconnect(Callable callback)
    {
        return IsValid && Owner.Disconnect(Name, callback);
    }

    public bool IsConnected(Callable callback)
    {
        return IsValid && Owner.IsConnected(Name, callback);
    }

    public bool Emit(params Variant[] arguments)
    {
        return IsValid && Owner.EmitSignal(Name, arguments);
    }

    public bool Equals(Signal other)
    {
        return ReferenceEquals(Owner, other.Owner) && string.Equals(Name, other.Name, StringComparison.Ordinal);
    }

    public override bool Equals(object obj)
    {
        return obj is Signal other && Equals(other);
    }

    public override int GetHashCode()
    {
        return HashCode.Combine(Owner, Name);
    }

    public static bool operator ==(Signal left, Signal right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(Signal left, Signal right)
    {
        return !left.Equals(right);
    }
}
