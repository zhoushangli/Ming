namespace Ming;

using System.Runtime.InteropServices;


[StructLayout(LayoutKind.Sequential)]
public struct EulerAngles
{
    public float Yaw;
    public float Pitch;
    public float Roll;

    public EulerAngles(float pitch, float yaw, float roll)
    {
        Pitch = pitch;
        Yaw = yaw;
        Roll = roll;
    }

    public override string ToString()
    {
        return $"EulerAngles(Pitch={Pitch}, Yaw={Yaw}, Roll={Roll})";
    }
}
