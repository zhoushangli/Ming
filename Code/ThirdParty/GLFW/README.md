# GLFW 3.4 local Win32 fix

`glfw3.lib` retains its existing objects except for `win32_window.obj`, rebuilt from
[the official GLFW 3.4 source](https://github.com/glfw/glfw/tree/3.4).

The modified `_glfwPollEventsWin32` checks the active HWND's process before reading
its `GLFW` property. Windows can temporarily share an active window across attached
input queues. The unmodified code then reads another process's `_GLFWwindow*`,
crashing while switching between the Editor and its game process.

The replacement code is:

```c
DWORD processID = 0;
GetWindowThreadProcessId(handle, &processID);
window = processID == GetCurrentProcessId() ? GetPropW(handle, L"GLFW") : NULL;
```

Rebuild the one object and update the library with:

```powershell
powershell -NoProfile -File Code/ThirdParty/GLFW/RebuildOwnedWindow.ps1
```

The script verifies the pinned source archive SHA-256 before compiling with the
existing release DLL runtime (`/MD`). It can also rebuild an already patched
library. No dependency download occurs during normal Ming builds. GLFW's original
zlib license and source notices remain in the source archive and bundled headers.
