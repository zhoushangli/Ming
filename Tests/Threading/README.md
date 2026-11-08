# L01：一个常驻工作线程

这是供逐步填写的 C++17 框架。当前可以编译，但尚未启动工作线程；运行会报告 `[INCOMPLETE]` 并返回退出码 2。

## 职责与 Godot 对照

- `Code/MingEngine/Core/OS/Thread.hpp/.cpp` 对照 Godot 的 `core/os/thread.h/.cpp`：只负责原生线程的启动、身份与等待退出。保留 `Start`、`GetId`、`GetCallerId`、`IsStarted`、`WaitToFinish` 命名，大小写遵循 Ming。
- `L01PersistentWorker.cpp` 中的 `PersistentWorker` 是本课的使用示例，保存整数队列、停止标志和结果。任务调度、空闲等待及排空后关闭属于它的职责。
- `std::mutex` 保护队列和停止状态；`std::condition_variable` 配合该状态完成等待与通知，等待必须检查条件。当前无需额外实现 Mutex 或 Semaphore 包装类。
- Godot 在 `core/core_bind.h/.cpp` 中另有继承 `RefCounted` 的脚本 Thread 包装层。这里先保持原生层独立，之后通过 Ming 的反射和绑定机制接入 C#；暂不注册类、生成 C# 或实现托管回调。

原生回调使用 `Callback(void* userdata)`。当前 `ID` 使用 `std::thread::id`；未来脚本层需要另行转换，例如 Godot 脚本 Thread 的 `get_id()` 返回字符串。当前 `Start` 用 `bool` 表示是否成功，未复制 Godot 的原生 ID 返回值、脚本 Error 返回值或优先级设置。

`IsStarted()` 表示有尚未 join 的线程句柄，即使线程函数已经返回也仍为 true。它不表示线程函数仍在执行，因此原生层没有增加 `IsAlive()`。

## 填写顺序

1. **线程生命周期**：填写 `Thread::Start` 和 `Thread::WaitToFinish`。运行 `--thread`，用立即返回的回调检查启动、线程 ID 和等待退出，之后再连接常驻循环。
2. **空闲与退出**：配套填写 `PersistentWorker::Start`、`Stop` 和 `ThreadFunction` 的空队列路径。先运行 `--empty`，验证等待中的线程可以被关闭唤醒。
3. **任务交接**：填写 `Submit` 和取任务逻辑。检查接单状态与入队使用同一把锁；关闭后提交返回 false。
4. **计算与结果**：在锁外计算整数平方，记录输入、64 位结果和实际执行线程 ID。
5. **排空后关闭**：确认关闭时已接收任务都会完成，再退出并 join。

TODO 数字对应以上职责。第 2 步需要一起处理启动、循环与停止，不能只启动线程就让实例析构。占位异常用于阻止未完成入口被误用；补完对应实现后删除该异常。线程入口中的 TODO 当前为空，完成后应由正常退出条件结束。

## 本课约定

- 一个实例只进行一轮启动与关闭。`Start`、`Submit`、`Stop` 和读取结果由创建它的主线程调用；暂不支持多个提交者或并发关闭。
- 只有工作线程写入结果，主线程在 `Stop` 完成 join 后读取，所以本课不增加结果队列。
- 关闭先停止接单，再通知工作线程；工作线程在没有剩余任务时退出。不要持有队列锁等待 join。
- 析构时会调用 `Stop`，它应能重复调用。底层 Thread 必须先 `WaitToFinish` 再销毁，禁止 detach；任务须能正常返回。
- 当前等待工具是条件变量。它不会储存通知次数，正确性依赖受同一把锁保护的队列和停止条件；这与计数信号量不同。

## 构建与运行

使用 `.vscode/tasks.json` 中同一套 MSBuild 工具链，从仓库根目录构建本课的独立入口：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' .\Tests\Threading\ThreadingLesson.vcxproj /m /nologo /verbosity:minimal /property:Configuration=Debug /property:Platform=x64
```

构建后复用同一产物：

```powershell
.\Temporary\ThreadingLesson\ThreadingLesson.exe --thread
.\Temporary\ThreadingLesson\ThreadingLesson.exe --empty
.\Temporary\ThreadingLesson\ThreadingLesson.exe
```

`--thread` 只检查原生线程生命周期；`--empty` 检查无任务关闭；默认入口提交 3、5、8 后立即关闭，检查平方结果、顺序、同一个不同于主线程的工作线程 ID，以及关闭后拒绝提交。退出码 0 表示该入口通过，1 表示失败，2 表示尚未填写。

现有入口不证明线程确实经历过阻塞等待，也不保证关闭恰好发生在队列非空时：线程调度可能让任务提前完成。补完后还需用断点或有同步控制的验证检查空闲阻塞与带积压任务的关闭。必要时由外部超时终止卡住的练习进程，不能把“尚未返回”视为通过。
