# Ming Engine

Ming Engine is a personal C++ game engine developed as a learning and research project. It focuses on building core engine systems from the ground up, including rendering, scene management, input, audio, editor tooling, and script integration.

## Features

- C++ engine architecture with modular core, engine, scene, editor, and service layers
- Direct3D 11 rendering backend
- Scene graph / node-based runtime structure
- 3D scene objects, cameras, lights, resources, and viewport management
- AngelScript integration for gameplay scripting
- Native C++ bindings exposed to script-side code
- Runtime script modules, script instances, and script resource identity tracking
- Editor tooling built with ImGui, including UI panels and transform gizmos
- Input and window management through GLFW
- Audio support through FMOD
- Asset and data loading support using tinygltf, tinyxml2, stb, and nlohmann json
- Math library with vectors, matrices, Euler angles, raycasts, bounds, and utility functions
- Debug rendering, bitmap fonts, post-processing pipeline, buffers, shaders, and texture handling
- Basic physics and scene import/resource systems

## Repository Layout

- `Code/MingEngine/Core` - math, object model, variants, and render primitives
- `Code/MingEngine/Engine` - application, rendering, input, window, audio, file, event, and scripting systems
- `Code/MingEngine/Scene` - node system, 3D scene types, resources, import, physics, and viewport logic
- `Code/MingEngine/Editor` - editor UI and gizmo tools
- `Code/MingEngine/EngineService` - shared engine services
- `Code/ThirdParty` - third-party libraries used by the engine

## Script System

Ming Engine uses AngelScript as its embedded scripting language. The script layer is designed to bridge C++ engine objects and script-side gameplay code through generated/native bindings, script modules, script instances, and resource identity tracking.

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

- 使用 C++ 构建的模块化引擎架构，包含 Core、Engine、Scene、Editor 和 EngineService 等层级
- 基于 Direct3D 11 的渲染后端
- 基于节点的场景结构和运行时对象管理
- 支持 3D 场景对象、摄像机、光源、资源和 Viewport 管理
- 集成 AngelScript 作为游戏逻辑脚本语言
- 将原生 C++ 引擎接口绑定并暴露给脚本侧使用
- 支持脚本模块、脚本实例和脚本资源身份追踪
- 使用 ImGui 构建编辑器工具，包括 UI 面板和变换 Gizmo
- 使用 GLFW 进行窗口和输入管理
- 使用 FMOD 提供音频支持
- 使用 tinygltf、tinyxml2、stb 和 nlohmann json 支持资源与数据加载
- 自研数学库，包含向量、矩阵、欧拉角、射线检测、包围体和常用工具函数
- 包含 Debug Renderer、Bitmap Font、后处理管线、Buffer、Shader 和 Texture 等渲染基础设施
- 包含基础物理、场景导入和资源系统

## 仓库结构

- `Code/MingEngine/Core` - 数学库、对象模型、Variant 和渲染基础类型
- `Code/MingEngine/Engine` - 应用层、渲染、输入、窗口、音频、文件、事件和脚本系统
- `Code/MingEngine/Scene` - 节点系统、3D 场景类型、资源、导入、物理和 Viewport 逻辑
- `Code/MingEngine/Editor` - 编辑器 UI 和 Gizmo 工具
- `Code/MingEngine/EngineService` - 引擎公共服务
- `Code/ThirdParty` - 项目使用的第三方库

## 脚本系统

Ming Engine 使用 AngelScript 作为嵌入式脚本语言。脚本系统用于连接 C++ 引擎对象和脚本侧游戏逻辑，包含原生绑定、脚本模块、脚本实例和资源身份追踪等机制。

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
