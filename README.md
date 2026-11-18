# Ming Engine

Ming Engine 是一个使用 C++ 编写的个人自研游戏引擎项目，主要用于学习、实验和构建完整的游戏引擎基础设施。项目覆盖渲染、场景管理、输入、音频、编辑器工具以及脚本系统等核心模块。

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

解决方案文件夹与上述结构对应。C++ 筛选器跟随源码目录，外部依赖统一归入 `ThirdParty`。`Ming.NET.Sdk` 和测试文件通过解决方案条目展示，不增加构建项目。

## 脚本系统

Ming Engine 通过 .NET 承载 C# 游戏脚本。`MingSharp` 提供托管 API，`MingPlugins` 处理托管加载与互操作，`MingTools` 提供项目及构建工具，`Ming.SourceGenerators` 提供编译期脚本元数据。

## 构建说明

当前仓库包含引擎项目文件和源码。项目主要在 Windows / Visual Studio 环境下开发，渲染后端基于 Direct3D 11。

### C# SDK 包

生成 C# API 绑定后，在引擎仓库根目录执行：

```powershell
dotnet pack Code/CSharp/Ming.NET.Sdk/Ming.NET.Sdk.csproj -c Release
```

此命令以 Release 构建托管 API 和源码生成器，输出 `Temporary/NuGet/Ming.NET.Sdk.0.1.0-dev.1.nupkg`。普通引擎构建不生成包。包内包含 SDK 构建规则、`MingSharp.dll`、API 文档和源码生成器，不包含引擎运行程序。

包名和版本统一维护在 `Code/CSharp/Ming.NET.Sdk/MingSdkPackage.props` 中，并在构建时嵌入 MingTools。新游戏项目使用 `<Project Sdk="Ming.NET.Sdk/0.1.0-dev.1">`，目标框架仍为 `net9.0`。生成器不会自动改写已有项目。

在 nuget.org 点击 **Upload** 上传 `.nupkg`，核对元数据并提交。等待校验和索引完成后，再执行 `dotnet build Game.csproj` 构建游戏项目。发布之前，全新环境无法从 nuget.org 还原此包；本流程不配置本地包源。打包、发布和运行验证分别确认。

已发布版本不能覆盖。API 或构建规则变化后，需要递增共享版本、重新构建 MingTools、生成新包，并明确更新已有游戏项目的 SDK 版本。包名能否使用以 nuget.org 的名称和所有权校验为准。

## 未来计划

- 实验基于 SDFGI 的全局光照方案
- 探索 Vulkan 渲染后端
- 实现简易动画系统
- 接入第三方物理引擎
- 完善编辑器体验，包括场景编辑、资源管理和运行时检查
- 构建游戏打包/导出流程

## 项目状态

这是一个持续开发中的个人引擎项目。随着架构演进，API 和系统设计可能会继续调整。
