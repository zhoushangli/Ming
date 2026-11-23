# Ming Engine

Ming Engine 是一个使用 C++ 编写的个人自研游戏引擎项目，主要用于学习、实验和构建完整的游戏引擎基础设施。项目覆盖渲染、场景管理、输入、音频、编辑器工具以及脚本系统等核心模块。

## UI Theme 圆角标准

默认 Theme 和编辑器 Theme 共用 `ThemeRadii` 半径，以 100% 为基准，编辑器按当前比例缩放。控件通过 StyleBox 和类型变体选择外观，业务面板不要自行硬编码圆角。

| 分类 | 半径 | 对象 |
| --- | ---: | --- |
| 连续结构 | 0 | 根背景、Dock 内容区、Viewport、结构栏、日志区、分隔条、滚动轨道、树连接线及拖放标记 |
| 小型交互 | 2 | 工具栏与菜单按钮、树节点悬停和选中背景、输出过滤与分组按钮、SpinBox 步进按钮 |
| 标准控件 | 4 | Button、OptionButton、LineEdit、属性编辑控件、滚动滑块、进度条背景 |
| 独立面板 | 6 | PanelContainer、Inspector 分组标题、ColorPicker 面板 |
| 浮动容器 | 8 | 菜单弹出面板、编辑器对话框外壳 |
| 拼接标签 | 上角 4，下角 0 | TabContainer 标签、场景标签、Output 标签 |

同一控件的 normal、hover、pressed、disabled、focus 采用相同轮廓，焦点样式只绘制边框。拼接区域保持直角；对话框预留外边框宽度，标题只圆上角，正文只圆下角，内部半径扣除外边框宽度。进度条填充的半径扣除背景内边距，绘制时按实际尺寸收缩。

`ThemeStyleBox` 提供 `GetCornerRadius(Corner)`、`SetCornerRadius(Corner, float)`、`SetCornerRadiusAll(float)`。四角默认是 0，负数归零，非有限值不接受。复制、移动和编辑器缩放保留圆角配置，修改共享样式会沿现有 Theme 通知链自动刷新控件。

圆角使用 CPU 生成的三角形，每角 8 段，边缘采用约 1 个渲染像素的透明过渡带。圆角不改变矩形布局、点击区域和子节点裁剪方式。新增控件应选用上述分类；连接区域只保留外露角，状态切换不改变半径。

验证与交互预览：

```powershell
.\Tests\RunRoundedThemeVerification.ps1
.\Tests\RunRoundedThemeVerification.ps1 -SkipBuild -Preview -Scale 1.25
.\Tests\RunRoundedThemeVerification.ps1 -SkipBuild -Capture -Scale 1.25
.\Tests\RunRoundedThemeVerification.ps1 -SkipBuild -EditorCapture -Scale 1.25
```

`-SkipBuild` 复用当前 Debug 引擎对象文件，验证程序自身仍会编译。`-Capture` 从 GPU 读取控件预览和组合弹窗，保存到 `Temporary/RoundedThemeVerification`，不代表真实桌面输入验收。也可直接运行 `Run/RoundedThemeVerification.exe --preview 1.25` 打开预览。

`-EditorCapture` 使用独立临时项目，检查主编辑器、Inspector、菜单和项目设置弹窗，并将渲染输出保存在同一目录。测试使用临时设置目录，不覆盖用户的编辑器设置。

## 技术特性

- 使用 C++ 构建的引擎架构，按 Core、Engine、Scene 和 Editor 组织
- 基于 Direct3D 11 的渲染后端
- 基于节点的场景结构和运行时对象管理
- 支持 3D 场景对象、摄像机、光源、资源和视口管理
- 通过 .NET 宿主与生成绑定支持 C# 游戏逻辑脚本
- 将原生 C++ 引擎接口绑定并暴露给脚本侧使用
- 支持脚本模块、脚本实例和脚本资源身份追踪
- 使用原生场景 GUI 控件构建编辑器工具，包括界面面板和变换操纵器
- 使用 GLFW 进行窗口和输入管理
- 使用 miniaudio 提供音频支持
- 使用 tinygltf、tinyxml2、stb 和 nlohmann json 支持资源与数据加载
- 自研数学库，包含向量、矩阵、欧拉角、射线检测、包围体和常用工具函数
- 包含调试渲染器、位图字体、后处理管线、缓冲区、着色器和纹理等渲染基础设施
- 包含基础物理、场景导入和资源系统

## 仓库结构

- `Code/MingEngine/Core` - 数学库、对象模型、Variant、操作系统基础能力和渲染基础类型
- `Code/MingEngine/Core/IO` - 虚拟路径、资源基类、加载、保存、导入、图像和 XML 工具
- `Code/MingEngine/Engine` - 应用层、渲染、输入、窗口、音频、文件、事件和脚本系统
- `Code/MingEngine/Scene` - 节点、GUI 控件、3D 场景类型、具体资源、SurfaceTool、导入器、物理和视口
- `Code/MingEngine/Editor` - 编辑器生命周期、主题、界面框架和操纵器工具
- `Code/MingEngine/Editor/UI/Panels` - 场景、文件系统、导入、输出和视口面板
- `Code/MingEngine/Editor/UI/Inspector` - 检查器面板与属性编辑器
- `Code/MingEngine/Editor/UI/Controls` - 编辑器可复用控件，包括 EditorSpinSlider
- `Code/CSharp` - 托管 API、程序集加载桥接、构建工具、源码生成器和 SDK
- `Code/ThirdParty` - 项目使用的第三方库
- `Tests` - 验证源码、运行脚本、测试数据和独立线程示例
- `Run` - 部署后的引擎程序与运行时文件
- `Temporary` - 构建中间文件与验证生成产物

解决方案文件夹与上述结构对应。C++ 筛选器跟随源码目录，外部依赖统一归入 `ThirdParty`。SDK 使用独立的 `Code/CSharp/SDK/Ming.NET.Sdk.sln`，包含 SDK、源码生成器和共享的 MingSharp 工程；主解决方案只构建引擎及其运行组件。

## 脚本系统

Ming Engine 通过 .NET 承载 C# 游戏脚本。`MingSharp` 提供托管 API，`MingPlugin` 处理托管加载、互操作、项目生成及构建工具，`Ming.SourceGenerators` 提供编译期脚本元数据。运行组件位于 `Code/CSharp/MingSharp/` 和 `Code/CSharp/MingPlugin/`；SDK 与源码生成器位于 `Code/CSharp/SDK/`。

## 构建说明

当前仓库包含引擎项目文件和源码。项目主要在 Windows / Visual Studio 环境下开发，渲染后端基于 Direct3D 11。

### VS Code 任务

- `Build MingEngine Debug` / `Build MingEngine Release`：构建并部署引擎及配套托管组件，不生成 NuGet 包。
- `Generate C# Bindings`：构建 Release 原生生成器，清理旧生成文件，再生成 MingSharp 绑定源码；不会自动编译新的托管 DLL。
- `Pack Ming SDK`：依次生成最新绑定、构建并部署 Release 托管组件、打包 SDK。任一步失败即停止，不自动上传。

任务依赖在 `tasks.json` 中串联；`launch.json` 用于启动或调试程序，并通过 `preLaunchTask` 运行准备任务。绑定生成的 launch 保留供调试使用。

`Run/MingSharp/Api/` 提供 MingPlugin、MingSharp 及其配套运行文件；游戏编译使用 NuGet SDK 包内的规则和生成器，不再使用旧的 `Run/MingSharp/Tool/`。

### C# SDK 包

完整准备流程使用 VS Code 的 `Pack Ming SDK` 任务。已有最新绑定时，也可在引擎仓库根目录单独打包：

```powershell
dotnet pack Code/CSharp/SDK/Ming.NET.Sdk/Ming.NET.Sdk.csproj -c Release
```

此命令以 Release 构建托管 API 和源码生成器，输出 `Temporary/NuGet/Ming.NET.Sdk.0.1.0-dev.1.nupkg`。普通引擎构建不生成包。包内包含 SDK 构建规则、`MingSharp.dll`、API 文档和源码生成器，不包含引擎运行程序。

包名和版本统一维护在 `Code/CSharp/MingSdkPackage.props` 中，并在构建时嵌入 MingPlugin。新游戏项目使用 `<Project Sdk="Ming.NET.Sdk/0.1.0-dev.1">`，目标框架仍为 `net9.0`。生成器不会自动改写已有项目。

在 nuget.org 点击 **Upload** 上传 `.nupkg`，核对元数据并提交。等待校验和索引完成后，再执行 `dotnet build Game.csproj` 构建游戏项目。发布之前，全新环境无法从 nuget.org 还原此包；本流程不配置本地包源。打包、发布和运行验证分别确认。

已发布版本不能覆盖。API 或构建规则变化后，需要递增共享版本、重新构建 MingPlugin、生成新包，并明确更新已有游戏项目的 SDK 版本。包名能否使用以 nuget.org 的名称和所有权校验为准。

## 未来计划

- 实验基于 SDFGI 的全局光照方案
- 探索 Vulkan 渲染后端
- 实现简易动画系统
- 接入第三方物理引擎
- 完善编辑器体验，包括场景编辑、资源管理和运行时检查
- 构建游戏打包/导出流程

## 项目状态

这是一个持续开发中的个人引擎项目。随着架构演进，API 和系统设计可能会继续调整。
