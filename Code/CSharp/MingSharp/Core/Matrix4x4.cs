using System.Runtime.InteropServices;

namespace Ming;

// TODO: The exact meaning of Matrix4x4 needs more discussion

[StructLayout(LayoutKind.Sequential)]
public struct Matrix4x4
{
    public float M11, M12, M13, M14;
    public float M21, M22, M23, M24;
    public float M31, M32, M33, M34;
    public float M41, M42, M43, M44;

    public override string ToString()
    {
        return $"Matrix4x4(M11={M11}, M12={M12}, M13={M13}, M14={M14}, " +
               $"M21={M21}, M22={M22}, M23={M23}, M24={M24}, " +
               $"M31={M31}, M32={M32}, M33={M33}, M34={M34}, " +
               $"M41={M41}, M42={M42}, M43={M43}, M44={M44})";
    }
}