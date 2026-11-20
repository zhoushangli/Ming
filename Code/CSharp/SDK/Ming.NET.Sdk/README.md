# Ming.NET.Sdk

面向 .NET 9 的 Ming C# 游戏项目所用的 MSBuild SDK，需要另行安装 .NET SDK。

```xml
<Project Sdk="Ming.NET.Sdk/0.1.0-dev.1">
  <PropertyGroup>
    <TargetFramework>net9.0</TargetFramework>
    <OutputType>Library</OutputType>
    <AssemblyName>Game</AssemblyName>
    <RootNamespace>Game</RootNamespace>
  </PropertyGroup>
</Project>
```

包内包含 MingSharp API 程序集及文档、脚本源码生成器和 SDK 构建规则。SDK 从 `Api/` 显式引用 API；它是 MSBuild SDK 包，而不是通过 `lib/` 目录提供程序集的常规类库包。

游戏构建输出位于 `.ming/dotnet/bin/<Configuration>/`。运行游戏需要兼容的 Ming 引擎版本，本包不包含引擎运行程序。本包为开发预览版，API 变化后需要同步更新包和引擎。

本 SDK 包内的 Ming 自有组件采用 MIT 许可证，见 `LICENSE.txt`；第三方组件保留各自版权和许可声明，见 `THIRD-PARTY-NOTICES.txt`。此许可声明仅适用于本 SDK 包，不为整个 Ming 引擎仓库指定许可证。
