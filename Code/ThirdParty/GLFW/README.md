# GLFW 3.4 本地 Win32 修复

`glfw3.lib` 保留了现有目标文件，仅使用 [GLFW 3.4 官方源码](https://github.com/glfw/glfw/tree/3.4)重新编译并替换了 `win32_window.obj`。

修改后的 `_glfwPollEventsWin32` 会在读取活动窗口句柄 HWND 的 `GLFW` 属性之前，检查窗口所属的进程。Windows 可能会在相互连接的输入队列之间临时共享活动窗口。原有代码因此可能读取另一个进程的 `_GLFWwindow*`，导致在编辑器与游戏进程之间切换时崩溃。

替换后的代码如下：

```c
DWORD processID = 0;
GetWindowThreadProcessId(handle, &processID);
window = processID == GetCurrentProcessId() ? GetPropW(handle, L"GLFW") : NULL;
```

使用以下命令重新编译该目标文件并更新静态库：

```powershell
powershell -NoProfile -File Code/ThirdParty/GLFW/RebuildOwnedWindow.ps1
```

脚本会先校验指定源码压缩包的 SHA-256，再使用现有的发布版 DLL 运行时（`/MD`）进行编译，也支持重新构建已经打过补丁的库。正常构建 Ming 时不会下载依赖。GLFW 原有的 zlib 许可证和源码声明保留在源码压缩包及随附的头文件中。
