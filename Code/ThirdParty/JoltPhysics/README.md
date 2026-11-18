# Ming 依赖目录结构

此目录包含 Jolt Physics 5.6.0 源码，以及预编译的 Windows x64 Debug 和 Release 静态库。

- `Jolt/`：上游头文件和实现源码，保留用于参考和调试。
- `Lib/x64/Debug/Jolt.lib`：Ming 的 `PhysicsServer.cpp` 链接的静态库。
- `Lib/x64/Debug/Jolt.pdb`：与静态库匹配的调试符号。
- `Lib/x64/Release/Jolt.lib`：使用 MSVC 14.38（VS 2022 17.8）构建的 Release 静态库，采用 `/MD` 并禁用 STL 迭代器调试。
- `Build/CMakeLists.txt`：Ming 使用保留源码构建 MSVC 静态库的最小构建配置，禁用了计算后端。
- `LICENSE`：上游 MIT 许可证。

上游构建脚本、示例、测试、资源、文档源码和仓库维护文件已移除。下方上游 README 译文中的相对链接可能指向未包含在此目录中的文件。完整仓库和构建说明请参阅[上游版本](https://github.com/jrouwe/JoltPhysics/tree/v5.6.0)。

随附的 Debug 静态库使用 MSVC v143、`/MDd`、AVX2、单精度位置和 Jolt 默认分配器。已启用调试渲染、性能分析、对象流和浮点异常支持；已禁用计算后端和过程间优化。对应的 Jolt 功能宏定义在 Ming 的 `PhysicsServer.cpp` 中。

`PhysicsServer.cpp` 根据 `_DEBUG` 选择 Debug 或 Release 静态库。两个静态库使用上述相同的功能宏；其运行时库和迭代器调试设置必须与引擎配置一致。

如需使用 Visual Studio 2026 随附的 CMake 和 VS 2022 v143 14.38 工具集重新构建 Release 静态库，请先在 VS 2022 中安装 14.38 工具集，再从仓库根目录运行以下 PowerShell 命令：

```powershell
$joltCmake = 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
& $joltCmake -S Code/ThirdParty/JoltPhysics/Build -B Temporary/JoltBuild-v143-14.38-VS2022 -G 'Visual Studio 17 2022' -A x64 -T 'v143,version=14.38.33130' '-DCMAKE_GENERATOR_INSTANCE=C:/Program Files/Microsoft Visual Studio/2022/Community'
& $joltCmake --build Temporary/JoltBuild-v143-14.38-VS2022 --config Release --parallel 6
```

请保持预编译 Release 静态库使用此工具集基线，以便 Ming 能够使用 MSVC 14.38 或更新版本链接。未指定版本的 `v143` 可能选择更新的编译器和 STL；生成的静态库可能依赖 `__std_search_1` 等符号，而 14.38 的运行时库不提供这些符号。

---

[![CLA 签署助手](https://cla-assistant.io/readme/badge/jrouwe/JoltPhysics)](https://cla-assistant.io/jrouwe/JoltPhysics)
[![构建状态](https://github.com/jrouwe/JoltPhysics/actions/workflows/build.yml/badge.svg)](https://github.com/jrouwe/JoltPhysics/actions/)
[![质量门禁状态](https://sonarcloud.io/api/project_badges/measure?project=jrouwe_JoltPhysics&metric=alert_status)](https://sonarcloud.io/dashboard?id=jrouwe_JoltPhysics)
[![缺陷](https://sonarcloud.io/api/project_badges/measure?project=jrouwe_JoltPhysics&metric=bugs)](https://sonarcloud.io/dashboard?id=jrouwe_JoltPhysics)
[![代码异味](https://sonarcloud.io/api/project_badges/measure?project=jrouwe_JoltPhysics&metric=code_smells)](https://sonarcloud.io/dashboard?id=jrouwe_JoltPhysics)
[![覆盖率](https://sonarcloud.io/api/project_badges/measure?project=jrouwe_JoltPhysics&metric=coverage)](https://sonarcloud.io/dashboard?id=jrouwe_JoltPhysics)

# Jolt Physics

一个适合多核并行的刚体物理与碰撞检测库，适用于游戏和虚拟现实应用。《地平线：西之绝境》和《死亡搁浅 2：冥滩之上》使用了此库。

[![《地平线：西之绝境》封面](https://jrouwe.nl/jolt/Horizon_Forbidden_West.png)](https://www.playstation.com/en-us/games/horizon-forbidden-west/)
[![《死亡搁浅 2》封面](https://jrouwe.nl/jolt/Death_Stranding_2.png)](https://www.playstation.com/en-us/games/death-stranding-2-on-the-beach/)

|[![布娃娃堆叠](https://img.youtube.com/vi/pwyCW0yNKMA/hqdefault.jpg)](https://www.youtube.com/watch?v=pwyCW0yNKMA)|
|:-|
|*展示使用 Jolt Physics 模拟布娃娃堆叠的 YouTube 视频。*|

更多演示和[视频](https://www.youtube.com/watch?v=pwyCW0yNKMA&list=PLYXVwtOr1CBxbA50jVg2dKUQvHW_5OOom)请参阅[示例](Docs/Samples.md)章节。

## 设计考量

为什么还要创建一个物理引擎？首先，这是作者的个人学习项目。其次，作者希望解决使用现有物理引擎时遇到的一些问题：

* 游戏除了物理模拟，还需要在多个线程中执行其他任务。因此，本库重视在主要模拟更新之外并发访问物理数据的能力：
    * 可以在后台加载或卸载部分模拟内容。在后台线程中批量准备物理体，无需加锁，也不会影响模拟；随后以尽可能小的性能开销将它们批量加入模拟。
    * 碰撞查询可以与物理体的添加、移除或更新并行执行。同一线程对物理体所做的修改会立即对查询可见；其他线程的修改则会让查询看到修改前或修改后的一致状态。另一种方案是分别维护世界的读版本和写版本，但这会使修改无法立即可见，因此本库未采用这种方式。
    * 碰撞查询可以与主要物理模拟并行执行。在模拟步骤之前进行粗略检测（宽阶段查询），然后在后台进行精细检测（窄阶段查询）。这样，导航网格生成等耗时任务就可以分散到多帧执行。
* 加载或卸载内容时，意外唤醒物理体会造成性能问题。因此，新建物理体不会自动将其唤醒，移除物理体也不会唤醒相邻物理体。如有需要，可以手动触发唤醒。
* 模拟具有确定性。只需复制模拟输入，就能在远程客户端重现模拟。有关限制，请阅读[确定性模拟](https://jrouwe.github.io/JoltPhysics/#deterministic-simulation)章节。
* 本库尝试模拟现实世界中的刚体行为，但会进行近似处理，因此主要面向游戏或虚拟现实模拟。

## 功能特性

* 使用连续碰撞检测模拟多种形状的刚体：
    * 球体
    * 盒体
    * 胶囊体
    * 渐缩胶囊体
    * 圆柱体
    * 渐缩圆柱体
    * 凸包
    * 平面
    * 复合形状
    * 三角网格
    * 地形（高度场）
* 模拟物理体之间的约束：
    * 固定约束
    * 点约束
    * 距离约束（包括弹簧）
    * 铰链约束
    * 滑动约束（也称移动副约束）
    * 圆锥约束
    * 齿轮齿条约束
    * 齿轮约束
    * 滑轮约束
    * 平滑样条路径约束
    * 摆动扭转约束（用于人形肩部）
    * 六自由度约束
* 使用马达驱动约束。
* 碰撞检测：
    * 射线投射。
    * 形状之间的相交检测。
    * 一个形状相对于另一个形状的扫掠检测。
    * 仅执行宽阶段检测，快速判断哪些对象可能相交。
* 传感器（触发区域）。
* 动画驱动的布娃娃：
    * 硬驱动（仅使用运动学刚体）。
    * 软驱动（为动态刚体设置速度）。
    * 通过约束马达驱动到动画姿态。
    * 在高精度动画骨架与低精度布娃娃骨架之间进行双向映射。
* 游戏角色模拟（胶囊体）：
    * 刚体角色：在物理模拟期间移动，开销最低，角色与动态物理体之间的碰撞响应最准确。
    * 虚拟角色：模拟中没有对应的刚体，而是使用碰撞检测来模拟角色。在物理更新之外执行更新，以提供更多控制，但与动态物理体的交互精度较低。
* 载具：
    * 轮式车辆。
    * 履带车辆。
    * 摩托车。
* 软体模拟（例如软球或布料）：
    * 边约束。
    * 二面角弯曲约束。
    * Cosserat 杆约束（带有朝向的边，可用于确定植物叶片等几何体的方向）。
    * 四面体体积约束。
    * 长距离附着约束（也称系绳约束）。
    * 将模拟限制在蒙皮顶点周围的指定范围内。
    * 内部压力。
    * 与模拟刚体碰撞。
    * 针对软体的碰撞检测。
* 在 GPU 上运行的发丝模拟：
    * 基于 Cosserat 杆。
    * 可以使用长距离附着约束限制发丝拉伸。
    * 支持模拟用的引导发丝和渲染用的跟随发丝。
    * 通过在网格中累积平均速度，并使用这些速度驱动发丝，处理发丝之间的碰撞。
    * 支持与环境碰撞，但目前仅支持 ConvexHull 和 CompoundShapes。
    * 发根可以通过蒙皮绑定到头皮网格。
* 水中浮力计算。
* 可选的双精度模式，支持大型世界。

## 支持的平台

* Windows x86/x64/ARM64
* Linux（在 Ubuntu 上测试）x86/x64/ARM32/ARM64/RISC-V64/LoongArch64/PowerPC64LE
* FreeBSD
* Android x86/x64/ARM32/ARM64
* Platform Blue（一款流行的游戏主机）x64
* macOS x64/ARM64
* iOS x64/ARM64
* MSYS2 MinGW64
* WebAssembly，请参阅[独立项目](https://github.com/jrouwe/JoltPhysics.js)。

## CPU 功能要求

* x86/x64 的最低要求为 SSE2，也可以使用 SSE4.1、SSE4.2、AVX、AVX2 或 AVX512 编译本库。
* ARM64 使用 NEON 和 FP16；ARM32 可以在不使用任何特殊 CPU 指令的情况下编译。

## 文档

入门请查看 [HelloWorld](HelloWorld/HelloWorld.cpp) 示例。另有[使用 CMake FetchContent 的 HelloWorld 示例](https://github.com/jrouwe/JoltPhysicsHelloWorld)，展示如何将 Jolt Physics 集成到 CMake 项目中。

Jolt 的每项功能都有对应示例。[运行示例应用](Docs/Samples.md)并浏览[源码](https://github.com/jrouwe/JoltPhysics/tree/master/Samples/Tests)，是学习此库的好方法。

更多信息请参阅最新的[架构与 API 文档](https://jrouwe.github.io/JoltPhysics/)，也可以查看[特定发布版本的文档](https://jrouwe.github.io/JoltPhysicsDocs/)。

作者在 GDC 2022 演讲《为〈地平线：西之绝境〉设计 Jolt Physics 架构》中详细介绍了部分算法（[幻灯片](https://gdcvault.com/play/1027560/Architecting-Jolt-Physics-for-Horizon)、[附演讲者备注的幻灯片](https://jrouwe.nl/architectingjolt/ArchitectingJoltPhysics_Rouwe_Jorrit_Notes.pdf)、[视频](https://gdcvault.com/play/1027891/Architecting-Jolt-Physics-for-Horizon)）。

## 编译

* 支持 Visual Studio 2022 及更新版本、Clang 16 及更新版本，或 GCC 12 及更新版本。
* 使用 C++17。
* 仅依赖标准模板库。
* 不使用运行时类型信息（RTTI）。
* 不使用异常。

如需在 Platform Blue 上运行，受保密协议要求限制，需要自行提供构建环境和 PlatformBlue.h。该文件可在 Platform Blue 开发者论坛获取。

构建说明请参阅[构建](Build/README.md)章节。从旧版本升级时，请参阅[发行说明](Docs/ReleaseNotes.md)或 [API 变更](Docs/APIChanges.md)章节。

## 性能

如需了解 Jolt 的多核扩展能力，以及与其他物理引擎的对比，请参阅[此文档](https://jrouwe.nl/jolt/JoltPhysicsMulticoreScaling.pdf)。

## 目录结构

* Assets：TestFramework、Samples 和 JoltViewer 使用的资源。
* Build：构建本库所需的全部内容，参阅[构建](Build/README.md)章节。
* Docs：本库的文档。
* HelloWorld：展示如何使用 Jolt Physics 的简单应用。
* Jolt：本库的全部源码。
* JoltViewer：可以使用 DebugRendererRecorder 类将物理引擎输出记录为 .jor 文件。此目录包含用于可视化回放记录的应用源码，例如可用于查看不同平台的 PerformanceTest 输出。目前支持 Windows、macOS 和 Linux。
* PerformanceTest：用于运行[性能测试](Docs/PerformanceTest.md)并收集耗时信息的简单应用。
* Samples：示例应用，参阅[示例](Docs/Samples.md)章节。目前支持 Windows、macOS 和 Linux。
* TestFramework：用于可视化物理引擎结果的渲染框架，供 Samples 和 JoltViewer 使用。目前支持 Windows、macOS 和 Linux。
* UnitTests：用于验证物理引擎行为的一组单元测试。

## 其他语言的绑定

* C：[实现一](https://github.com/amerkoleci/joltc)、[实现二](https://github.com/zig-gamedev/zphysics/tree/main/libs/JoltC)和[实现三](https://github.com/SecondHalfGames/JoltC/)
* [C#](https://github.com/amerkoleci/JoltPhysicsSharp)
* Java 或 Kotlin：[实现一](https://stephengold.github.io/jolt-jni-docs)和[实现二](https://github.com/Morgoth398/JoltPhysics-JavaFFM)
* [JavaScript](https://github.com/jrouwe/JoltPhysics.js)
* [Rust](https://github.com/SecondHalfGames/jolt-rust)
* [Python](https://github.com/Evilpasture/Culverin)
* [Zig](https://github.com/zig-gamedev/zphysics)

## 与其他引擎的集成

* [Godot](https://github.com/godotengine/godot)
* [Source Engine](https://github.com/Joshua-Ashton/VPhysics-Jolt)
* Unreal 插件：[实现一](https://github.com/OversizedSunCoreDev/ArtilleryEco)和[实现二](https://github.com/Yadhu-S/UnrealJolt)

使用 Jolt Physics 的项目请参阅[项目列表](Docs/ProjectsUsingJolt.md)。

## 许可证

本项目采用 [MIT 许可证](LICENSE)分发。

## 贡献

欢迎所有贡献！如计划进行较大改动，请先在 GitHub 讨论区沟通。对于非简单改动，需要同意[贡献者协议](ContributorAgreement.md)。创建拉取请求（PR）时，[CLA 签署助手](https://cla-assistant.io/)会提示签署协议。

所有 PR 都会在合并前压缩为一个提交，因此无需通过强制推送来整理提交历史。
