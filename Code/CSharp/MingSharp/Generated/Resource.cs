namespace MingSharp
{
	public partial class Resource : RefCounted
	{
		internal Resource(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
