# Ming Engine

Ming Engine is a personal C++ game engine developed as a learning and research project. It focuses on building core engine systems from the ground up, including rendering, scene management, input, audio, editor tooling, and script integration.

## Features

- C++ engine architecture organized into Core, Engine, Scene, and Editor
- Direct3D 11 rendering backend
- Scene graph / node-based runtime structure
- 3D scene objects, cameras, lights, resources, and viewport management
- C# gameplay scripting through the .NET host and generated bindings
- Native C++ bindings exposed to script-side code
- Runtime script modules, script instances, and script resource identity tracking
- Editor tooling built with native scene GUI controls, including UI panels and transform gizmos
- Input and window management through GLFW
- Audio support through miniaudio
- Asset and data loading support using tinygltf, tinyxml2, stb, and nlohmann json
- Math library with vectors, matrices, Euler angles, raycasts, bounds, and utility functions
- Debug rendering, bitmap fonts, post-processing pipeline, buffers, shaders, and texture handling
- Basic physics and scene import/resource systems

## Repository Layout

- `Code/MingEngine/Core` - math, object model, variants, OS primitives, and render primitives
- `Code/MingEngine/Core/IO` - virtual paths, base resources, loading, saving, importing, images, and XML utilities
- `Code/MingEngine/Engine` - application, rendering, input, window, audio, file, event, and scripting systems
- `Code/MingEngine/Scene` - nodes, GUI controls, 3D scene types, concrete resources, SurfaceTool, importers, physics, and viewports
- `Code/MingEngine/Editor` - editor lifecycle, themes, UI shell, and gizmo tools
- `Code/MingEngine/Editor/UI/Panels` - scene, filesystem, import, output, and viewport panels
- `Code/MingEngine/Editor/UI/Inspector` - Inspector panel and property editors
- `Code/MingEngine/Editor/UI/Controls` - reusable editor controls, including EditorSpinSlider
- `Code/CSharp` - managed API, assembly loading bridge, build tools, source generator, and SDK
- `Code/ThirdParty` - third-party libraries used by the engine
- `Tests` - verification sources, runners, fixtures, and standalone threading lessons
- `Run` - deployed engine executables and runtime files
- `Temporary` - build intermediates and generated verification artifacts

Solution folders follow this layout. C++ filters mirror source directories, with external dependencies grouped under `ThirdParty`. `Ming.NET.Sdk` and test files are shown as solution items without adding build projects.

## Script System

Ming Engine hosts C# gameplay scripts through .NET. `MingSharp` exposes the managed API, `MingPlugins` handles managed loading and interop, `MingTools` provides project/build tools, and `Ming.SourceGenerators` supplies compile-time script metadata.

## Build Notes

This repository currently contains the engine project files and source code. The project is primarily developed on Windows with Visual Studio and a Direct3D 11 rendering backend.

## Roadmap

- SDFGI-based global illumination experiments
- Vulkan rendering backend exploration
- Simple animation system
- Integration with a third-party physics engine
- More complete editor workflow, including scene editing, asset management, and runtime inspection
- Packaging/export pipeline for playable builds

## Status

This is an actively developed personal engine. APIs and systems may change as the architecture evolves.

# Ming Engine

Ming Engine 是一个使用 C++ 编写的个人自研游戏引擎项目，主要用于学习、实验和构建完整的游戏引擎基础设施。项目覆盖渲染、场景管理、输入、音频、编辑器工具以及脚本系统等核心模块。

## 技术特性

- 使用 C++ 构建的引擎架构，按 Core、Engine、Scene 和 Editor 组织
- 基于 Direct3D 11 的渲染后端
- 基于节点的场景结构和运行时对象管理
- 支持 3D 场景对象、摄像机、光源、资源和 Viewport 管理
- 通过 .NET 宿主与生成绑定支持 C# 游戏逻辑脚本
- 将原生 C++ 引擎接口绑定并暴露给脚本侧使用
- 支持脚本模块、脚本实例和脚本资源身份追踪
- 使用原生场景 GUI 控件构建编辑器工具，包括 UI 面板和变换 Gizmo
- 使用 GLFW 进行窗口和输入管理
- 使用 miniaudio 提供音频支持
- 使用 tinygltf、tinyxml2、stb 和 nlohmann json 支持资源与数据加载
- 自研数学库，包含向量、矩阵、欧拉角、射线检测、包围体和常用工具函数
- 包含 Debug Renderer、Bitmap Font、后处理管线、Buffer、Shader 和 Texture 等渲染基础设施
- 包含基础物理、场景导入和资源系统

## 仓库结构

- `Code/MingEngine/Core` - 数学库、对象模型、Variant、OS 基础能力和渲染基础类型
- `Code/MingEngine/Core/IO` - 虚拟路径、资源基类、加载、保存、导入、图像和 XML 工具
- `Code/MingEngine/Engine` - 应用层、渲染、输入、窗口、音频、文件、事件和脚本系统
- `Code/MingEngine/Scene` - 节点、GUI 控件、3D 场景类型、具体资源、SurfaceTool、导入器、物理和 Viewport
- `Code/MingEngine/Editor` - 编辑器生命周期、主题、UI 外壳和 Gizmo 工具
- `Code/MingEngine/Editor/UI/Panels` - 场景、文件系统、导入、输出和视口面板
- `Code/MingEngine/Editor/UI/Inspector` - Inspector 面板与属性编辑器
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

## 未来计划

- 实验基于 SDFGI 的全局光照方案
- 探索 Vulkan 渲染后端
- 实现简易动画系统
- 接入第三方物理引擎
- 完善编辑器体验，包括场景编辑、资源管理和运行时检查
- 构建游戏打包/导出流程

## 项目状态

这是一个持续开发中的个人引擎项目。随着架构演进，API 和系统设计可能会继续调整。
