namespace Ming;

public partial class InputSystem
{
    public static InputSystem GetInput()
    {
        return GetGlobalObject<InputSystem>("InputSystem");
    }
}
