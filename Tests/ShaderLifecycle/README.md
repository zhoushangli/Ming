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

Place `uniform` declarations before ordinary HLSL definitions. Supported types are `int`, `float`, `float2`, `float3`, `float4`. Integers remain signed 32-bit HLSL integers. Defaults accept signed numeric literals and vector constructors with matching numeric components; omitted defaults are zero.

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

Types strictly match `int`, `float` and `Vector2/3/4`. Unknown names or mismatched types return false and preserve the previous value. RenderServer packs numeric values into each RenderRequest when it is collected. Renderer uploads that snapshot to a reusable constant buffer at b6 before each draw and unbinds b6 for requests without material constants. Material changes take effect in newly collected requests. Materials sharing a Shader retain independent values. Shader replacement or resource reload preserves matching names and types. New parameters and type changes use the new declaration defaults; removed parameters are discarded.

The parser checks top-level declarations, callback signatures and delimiter boundaries without an AST. Ordinary HLSL and preprocessor directives pass through. MShader declarations and callbacks must appear directly in the main file, independently of macros, conditional compilation and include files. No canvas_item, texture uniform declarations, arrays or matrices are provided. Shader parameters support Inspector editing and material resource persistence.

Run `Tests/RunShaderLifecycleVerification.ps1 -SkipBuild` after a Debug build. It checks parser failures, defaults, type validation, resource reload and GPU readback of six meshes, including runtime updates and shared-Shader materials. Add `-Preview` for a visible preview. `Bad.mshader` intentionally exercises compiler failure diagnostics.

After running the verification script, double-click `Run/ShaderLifecyclePreview.exe` to reopen the generated preview project. Close its window to exit. The executable is a local build artifact.

## ???? / Material resources

`MaterialResource` ????? Shader?????????????????????????????`StandardMaterial2D` ?? Tint?Blend?Filter?Repeat??????????`StandardMaterial3D` ????????????????`ShaderMaterial` ?? Shader??????????????Standard ????????? Shader?

CanvasItem ??????????????????? 2D ????????????????? CanvasItem Filter/Repeat ?????????? StandardMaterial2D???? SetMaterial ???? MaterialResource ?????????????????????????

Shader ????????????????????????????????????????????????????????? GPU ????????????????????????????

The base owns common storage and exposes no material configuration API. StandardMaterial2D exposes Tint, Blend, Filter and Repeat; draw commands supply textures. StandardMaterial3D exposes fixed surface parameters and draw states. ShaderMaterial exposes shader selection, named parameters and draw states. CanvasItem uses its assigned material or the engine default 2D material, without parent material inheritance. External legacy resources require migration to concrete material types.

The visible shader preview also contains 2D default/nearest/tinted samples and a StandardMaterial3D sample. Run the editor resource checks with `Tests/RunEditorProjectVerification.ps1 -SkipBuild -ResourcesOnly`; run managed material checks with `Tests/RunApplicationLifecycleVerification.ps1 -SkipBuild -Modules --verify-managed`.

## Shader 参数 Inspector / Shader parameter Inspector

选中 ShaderMaterial 后，自定义 uniform 按声明顺序及分组显示。普通 float2/3/4 使用分量数值框；hint_color、hint_range 和 hint_enum 分别提供颜色、范围和枚举编辑。拖动实时更新材质预览，松开提交一条撤销记录；恢复默认按钮清除显式覆盖。材质只保存覆盖值，未覆盖参数跟随 shader 默认值；等于默认值的显式覆盖仍可恢复默认。

Selecting a ShaderMaterial exposes custom uniforms in declaration and group order. Ordinary vectors use component fields; hint_color, hint_range and hint_enum select color, range and enum editors. Drags preview live and commit one undo action. Restore Default removes the explicit override. Only overrides are serialized; unmodified parameters follow shader defaults. An explicit value equal to the default remains an override.

```powershell
# 自动验证 / Automated checks
.\Tests\RunEditorProjectVerification.ps1 -SkipBuild -ResourcesOnly
# 打开已选中示例材质的编辑器 / Open the editor with the sample material selected
.\Tests\RunEditorProjectVerification.ps1 -SkipBuild -PreviewShaderParameters
```

交互预览每次在 `Temporary/ShaderParameterPreview` 重新生成独立示例。可调整范围、浮点颜色、枚举及普通数值，撤销重做、恢复默认和保存；关闭窗口退出。提示相关截图位于 `Temporary/ShaderParameterEvidence`。

The interactive preview regenerates an isolated sample in `Temporary/ShaderParameterPreview`. Edit ranges, floating-point colors, enums and ordinary numbers, undo/redo, restore defaults, or save; close the window to exit. Hint screenshots are stored in `Temporary/ShaderParameterEvidence`.

MaterialResource stores user parameters and render states without owning a GPU constant buffer. RenderRequest contains resolved draw data and owns its material parameter bytes; it has no MaterialResource reference. Canvas draw-command textures are resolved through ApplyMaterialToRequest without changing the shared material.

The engine preview also checks alternating 16-byte and 64-byte material layouts, b6 unbinding in both stages, parameter snapshots when a later request changes an earlier material, and distinct Canvas textures sharing one material. SmallUniforms.mshader keeps the vertex-deformation preview while exercising the smaller layout.


## Uniform hints / 参数提示

```hlsl
shader_type spatial;

group_uniforms Appearance;
uniform float opacity : hint_range(0, 1, 0.01) = 0.8;
uniform float4 tint : hint_color = float4(0.12345, 0.5, 0.8, 1);

group_uniforms Effect;
uniform int mode : hint_enum("Normal", "Glow", "Outline") = 0;
uniform int count : hint_range(0, 16) = 4;

group_uniforms;
uniform float strength = 1;

void fragment(inout FragmentContext f)
{
    f.color = tint * opacity * strength;
}
```

`hint_range(min, max[, step])` supports scalar float/int. Omitted steps are 0.001/1. Bounds must be ordered and the step positive; int ranges use integer literals. The Inspector enforces the editing range; runtime setters retain strict types without clamping values.

`hint_color` applies only to float4. Material properties, saved overrides and C++/C# APIs use Color with four 32-bit floating-point channels; ordinary float4 uses Vector4. RGB numeric fields accept negative and above-one values. HSV and Hex editing require ordinary-range RGB (Hex also requires ordinary-range alpha); previews clamp a copy without changing the value. Alpha editing uses 0-1. Hex and byte images round to 8-bit channels at conversion boundaries. Vertex and uniform uploads preserve the floating-point values. Existing UNORM render targets still clamp and quantize when written; this change does not add HDR rendering or color-space conversion.

`hint_enum("A", "B")` applies only to int, with values 0, 1, etc. Labels must be nonempty and cannot contain commas, colons or escapes. One hint is allowed per parameter. Explicit enum values and nested groups are not supported.

`group_uniforms Name;` starts or switches a single-level Inspector group. `group_uniforms;` ends it; subsequent parameters return to Shader Parameters. Grouping never changes parameter names or GPU storage. All declarations precede ordinary HLSL definitions.

Each constant has a 16-byte slot; m_byteSize records only its payload: int/float=4, float2=8, float3=12, float4=16 bytes.

颜色参数与普通颜色统一使用四浮点 Color，普通 float4 仍使用 Vector4。RGB 数值允许负数和大于 1；扩展 RGB 禁用 HSV、Hex 编辑，Alpha 编辑限制为 0～1，预览仅截断副本。Hex 和字节图片在转换边界量化；顶点及 uniform 上传保留原值，中间 UNORM 目标仍在写入时截断，不引入 HDR 流程或颜色空间转换。旧字节颜色数据及颜色 uniform 的 Vector4 数据需迁移为新格式，不自动猜测旧数值含义。单层分组不改变参数名，所有声明仍须位于 HLSL 定义之前。

范围提示仅用于 float/int，省略步长时分别使用 0.001/1；它限制面板编辑，不裁剪运行时 setter 的值。枚举提示仅用于 int，非空选项依次对应 0、1、2，不支持逗号、冒号或转义字符。一个参数只接受一个提示。颜色控件保留浮点 RGBA 精度，十六进制和屏幕取色沿用来源的 8 位精度。

Focused verification and a directly openable editor preview:

```powershell
.\Tests\RunEditorProjectVerification.ps1 -SkipBuild -ShaderHintsOnly
.\Tests\RunEditorProjectVerification.ps1 -SkipBuild -PreviewShaderParameters
```

The preview project is generated under `Temporary/ShaderParameterPreview/`; screenshots are stored in `Temporary/ShaderParameterEvidence/`. Use `-PreviewShaderParameters -BuildOnly` to prepare `Run/ShaderParameterPreview.exe`, which can then be opened directly. Shader/GPU checks use `Tests/RunShaderLifecycleVerification.ps1 -SkipBuild`.

使用 `-PreviewShaderParameters -BuildOnly` 可生成能直接双击打开的 `Run/ShaderParameterPreview.exe`。测试项目和本地设置均写入 Temporary，截图单独保存，避免作为项目资源导入。
