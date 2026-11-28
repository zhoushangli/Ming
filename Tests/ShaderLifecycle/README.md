# Ming Shader 生命周期

资源加载使用 `.mshader`，首个声明必须为 `shader_type spatial;`。两个普通 HLSL 回调均可省略，引擎会补空函数。引擎自动提供 Context、常量和 GPU 入口，不需要用户调用默认初始化函数。

```hlsl
shader_type spatial;

void vertex(inout VertexContext v)
{
    v.modelPosition.y += 1.0;
}

void fragment(inout FragmentContext f)
{
    f.color.rgb = float3(1.0, 0.0, 0.0);
}
```

原始位置 `(2, 3, 4)` 在 `vertex()` 中成为 `(2, 4, 4)`，引擎随后计算世界、相机、渲染和裁剪坐标。这里的 Y 是模型空间 Y，会受到模型旋转和缩放影响。变形不会自动重算法线或切线。

## 字段和执行时机

| Context | 字段 | 权限 | 默认值和写入效果 |
|---|---|---|---|
| Vertex | `float3 modelPosition` | 读写 | Mesh 模型空间位置，写入后参与坐标变换 |
| Vertex | `float3 modelNormal` | 读写 | Mesh 模型空间法线，写入后参与方向变换 |
| Vertex | `float3 modelTangent` | 读写 | Mesh 模型空间切线，写入后参与方向变换 |
| Vertex | `float3 modelBitangent` | 读写 | Mesh 模型空间副切线，写入后参与方向变换 |
| Vertex | `float2 uv` | 读写 | Mesh UV，插值后用于默认采样 |
| Vertex | `float4 color` | 读写 | Mesh RGBA，插值后参与默认表面颜色 |
| Fragment | `float3 worldPosition` | 只读约定 | 变形后的插值世界位置 |
| Fragment | `float3 worldNormal` | 读写 | 插值世界法线，写入后用于光照与场景法线输出 |
| Fragment | `float2 uv` | 只读约定 | 插值 UV，默认采样已完成 |
| Fragment | `float4 vertexColor` | 只读约定 | 插值顶点 RGBA |
| Fragment | `float4 color` | 读写 | Diffuse 采样 × 顶点颜色 × ModelColor；光照前的表面 RGBA |

执行顺序：初始化顶点 → `vertex(v)` → 坐标及方向变换 → 光栅化与插值 → 默认 Diffuse 采样和颜色组合 → `fragment(f)` → 光照 → Alpha 裁剪 → 颜色及场景法线输出。

`fragment()` 后不会再自动乘一次纹理、顶点颜色或 Tint。写入红色仍会受到光照影响；A 小于 `0.01` 时裁剪，等于 `0.01` 时保留，也可以主动调用 HLSL `clip()`。

只读是接口约定，不是 HLSL 成员级编译限制。修改 `f.uv`、`f.vertexColor` 不会重做默认采样或颜色组合；后续引擎光照使用保存的原始世界位置，不消费修改后的 `f.worldPosition`。可在回调中自行采样 `DiffuseTexture.Sample(DiffuseSampler, ...)` 并赋给 `f.color`。

法线变换沿用当前实现。光照函数内部会归一化法线，场景法线输出沿用原始编码方式；自行写入法线时应提供单位世界法线。Normal 和 SGE 贴图的默认采样、额外表面参数不在本次接入范围。

公共只读数据包括 `ModelToWorldTransform`、`ModelColor`、相机矩阵、`ScreenSize`、`CameraNear`、`CameraFar`、`Time`、`DeltaSeconds` 和现有灯光数据。`Time`/`DeltaSeconds` 来自当前系统时钟。纹理和常量继续由现有绘制路径绑定。

## 迁移完整 Shader

通过资源加载器加载的 `.mshader` 统一是 3D Lit 函数式 Shader，不接受完整阶段入口。把模型空间顶点修改放进 `vertex()`，把光照前表面处理放进 `fragment()`。重复的 Context、常量、入口声明会导致编译冲突。预处理指令原样交给 HLSL 编译器；MShader 声明和回调不能依赖预处理结果。编译错误通过 `#line` 对应用户原始行号。

引擎内部直接调用 `CreateShader()` 仍接收完整 HLSL，入口统一为 `VertexMain` 和 `FragmentMain`。原 `PixelMain`、`VertexToPixel`、`PixelOutput` 分别改名为 `FragmentMain`、`VertexToFragment`、`FragmentOutput`。D3D11 API 类型名称及 `ps_5_0` 保留。

## 验证和预览

在仓库根目录运行：

```powershell
.\Tests\RunShaderLifecycleVerification.ps1
# 已完成同一版本 Debug 构建时复用产物：
.\Tests\RunShaderLifecycleVerification.ps1 -SkipBuild
# 打开真实引擎窗口，关闭窗口退出：
.\Tests\RunShaderLifecycleVerification.ps1 -SkipBuild -Preview
```

验证包含 D3D11 WARP 编译、顶点流输出、颜色/法线附件读回，以及真实引擎通过资源加载器加载示例并绘制网格。预览从左至右显示空回调、模型 Y 上移 1 单位、光照前改红；`preview.ppm` 输出至 `Temporary/ShaderLifecycleVerification/`。测试项目和编译产物也只写入 Temporary 与 Run。

同目录 `Empty.mshader`、`MoveUp.mshader`、`Red.mshader`、`Uniforms.mshader` 是可直接复用的最小示例。


## MShader parameters

Place `uniform` declarations before ordinary HLSL definitions. Supported types are `float`, `float2`, `float3`, `float4`. Defaults accept signed numeric literals and vector constructors with matching numeric components; omitted defaults are zero.

```hlsl
shader_type spatial;
uniform float height = 0.5;
uniform float3 tint = float3(1.0, 0.0, 0.0);

void vertex(inout VertexContext v)
{
    v.modelPosition.y += height;
}

void fragment(inout FragmentContext f)
{
    f.color.rgb = tint;
}
```

```cpp
auto shader = Ref<ShaderResource>(ResourceLoader::Load("res://Uniforms.mshader"));
auto material = CreateRef<ShaderMaterial>();
material->SetShaderResource(shader);
material->SetShaderParameter("height", 1.0f);
material->SetShaderParameter("rgb", Vector3(1.0f, 0.0f, 0.0f));
meshInstance->SetMaterialOverride(material);
```

Types strictly match `float` and `Vector2/3/4`. Unknown names or mismatched types return false and preserve the previous value. Changes mark the material dirty; drawing packs and uploads values. Materials sharing a Shader retain independent values. Shader replacement or resource reload preserves matching names and types. New parameters and type changes use the new declaration defaults; removed parameters are discarded.

The parser checks top-level declarations, callback signatures and delimiter boundaries without an AST. Ordinary HLSL and preprocessor directives pass through. MShader declarations and callbacks must appear directly in the main file, independently of macros, conditional compilation and include files. No canvas_item, texture parameters, arrays, matrices, Inspector parameter editing or parameter persistence are provided.

Run `Tests/RunShaderLifecycleVerification.ps1 -SkipBuild` after a Debug build. It checks parser failures, defaults, type validation, resource reload and GPU readback of six meshes, including runtime updates and shared-Shader materials. Add `-Preview` for a visible preview. `Bad.mshader` intentionally exercises compiler failure diagnostics.

After running the verification script, double-click `Run/ShaderLifecyclePreview.exe` to reopen the generated preview project. Close its window to exit. The executable is a local build artifact.

## ???? / Material resources

`MaterialResource` ????? Shader?????????????????????????????`StandardMaterial2D` ?? Tint?Blend?Filter?Repeat??????????`StandardMaterial3D` ????????????????`ShaderMaterial` ?? Shader??????????????Standard ????????? Shader?

CanvasItem ??????????????????? 2D ????????????????? CanvasItem Filter/Repeat ?????????? StandardMaterial2D???? SetMaterial ???? MaterialResource ?????????????????????????

Shader ????????????????????????????????????????????????????????? GPU ????????????????????????????

The base owns common storage and exposes no material configuration API. StandardMaterial2D exposes Tint, Blend, Filter and Repeat; draw commands supply textures. StandardMaterial3D exposes fixed surface parameters and draw states. ShaderMaterial exposes shader selection, named parameters and draw states. CanvasItem uses its assigned material or the engine default 2D material, without parent material inheritance. External legacy resources require migration to concrete material types.

The visible shader preview also contains 2D default/nearest/tinted samples and a StandardMaterial3D sample. Run the editor resource checks with `Tests/RunEditorProjectVerification.ps1 -SkipBuild -ResourcesOnly`; run managed material checks with `Tests/RunApplicationLifecycleVerification.ps1 -SkipBuild -Modules --verify-managed`.
