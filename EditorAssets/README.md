# Ming 编辑器图标

这里保存 Ming 编辑器图标的 SVG 源文件、`manifest.json` 清单，以及生成器依赖的两个品牌图片。恢复的 254 个 SVG 来自删除提交 `48113805` 的父提交中 `EditorStyleTest/warm-carbon/icons` 的最后版本。另外为现有 `Dropdown.png` 补回了像素效果一致的 `Dropdown.svg` 和清单条目，并新增 `StandardMaterial2D.svg`，当前共 256 个图标。

## 视觉特点

图标以 Godot 的概念和轮廓为参考，配合 Ming 的 Warm Carbon 主题。优先让用户在小尺寸下辨认含义，同族图标共用母题；同一概念的 2D、3D 版本优先通过类型色区分，只有几何含义不同才改变形状。

- **尺寸**：使用 `viewBox="0 0 16 16" width="16" height="16"`，按 16×16 网格设计。生成器将其栅格化为 32×32 透明 PNG；应同时检查 16、20、24px 显示效果。
- **主次层**：主层说明“是什么”，补充信息使用 `fill-opacity="0.45"` 的淡层。主次层之间保留约 1 个 SVG 单位的透明断笔间隙，可用蒙版实现。动作图标通常只用一层。
- **抽象类型**：使用淡层主体和细轮廓，如 `CanvasItem.svg`，与可实例化类型区分。
- **形状**：较大色块圆角约 3–4，方框外角约 2、内角保持直角，小部件圆角约 1，线条使用圆头。填充路径、描边和蒙版均可使用，不要求统一改成线性图标。
- **细节**：保留紧凑、清晰的几何轮廓。书法笔触只用于品牌标志和横笔，小图标不加入飞白、复杂纹理或装饰性背景。仅类型图标允许简短的维度角标（如 2D / 3D），必须转成路径，不使用 SVG 文本或依赖字体。

参考文件：动作图标 [Save.svg](Save.svg)、类型图标 [Node2D.svg](Node2D.svg)、材质图标 [StandardMaterial2D.svg](StandardMaterial2D.svg) / [StandardMaterial3D.svg](StandardMaterial3D.svg)、抽象类型 [CanvasItem.svg](CanvasItem.svg)。

标准材质沿用 ShaderMaterial 的同一圆盘主体（中心 8,8，半径 7，45% 淡层，无外轮廓），在右下角分别标注路径绘制的 2D / 3D；ShaderMaterial 的波形标记也位于同一右下角区域。三者共用角标的透明间隙，不添加底框。材质家族不采用 CanvasItem 的淡层加细轮廓样式。它们属于资源，均使用 `tn` 暖灰色，不按名称中的 2D / 3D 改成节点类型色。

## 颜色与清单

显示用的路径使用 `currentColor`，由引擎主题染色。蒙版内的 `#fff`、`#000` 用于计算可见区域，不是图标的显示配色。生成器保留 Alpha，将 PNG 的 RGB 统一为白色，因此不能通过在 SVG 内写死 RGB 来制作多色图标。

`manifest.json` 的键必须与 SVG 文件名（不含扩展名）一一对应，大小写保持一致。每项包含：

| 字段 | 含义 |
| --- | --- |
| `family` | 图标所属语义家族，沿用现有清单分组 |
| `tone` | 主题颜色键，写入生成头文件供运行时使用 |
| `abstract` | 是否代表抽象类型；视觉形状仍需在 SVG 中绘制 |

当前 Warm Carbon 主题在 `EditorThemeManager.cpp` 中定义以下颜色，实际效果以主题为准：

| tone | 用途 | 当前颜色 |
| --- | --- | --- |
| `tn` | 通用操作、其余节点和资源 | 暖灰 `#a49e93`（muted） |
| `t2d` | 2D 类型 | 青蓝 `#93d0dd` |
| `t3d` | 3D 类型 | 淡紫 `#a68bd4` |
| `tui` | Control / UI 类型 | 灰绿 `#9fbea0` |
| `terr` | 错误状态 | 柔红 `#e39898` |
| `twarn` | 警告状态 | 琥珀 `#e2ba72` |
| `tsuc` | 成功状态 | 淡绿 `#91b99b` |

类型图标的颜色跟随继承关系；缺少专用图标时，运行时会沿已注册的父类查找。场景树选中行保留类型色，通过行背景表示选中；不要把选中类型统一改成琥珀色。`family` 和 `abstract` 是源清单信息，目前不会由生成器自动转成形状或额外的运行时类型信息。

## 新增和生成

1. 参考同族现有文件，在本目录新增 `<IconName>.svg`。类型图标名应匹配引擎注册类名，操作图标名应匹配调用方使用的图标键；不要使用仅大小写不同的名称。
2. 在 `manifest.json` 中添加同名条目，例如新增具体 2D 类型时使用 `"tone": "t2d"`、`"abstract": false`，并沿用适合的 `family`。`tone` 应按实际类型职责选择，资源不要仅因名称含有 2D 就自动使用 `t2d`。
3. 从仓库根目录执行以下命令。首次使用时安装依赖，已有环境无需重复安装。

```powershell
python -m pip install Pillow playwright
python -m playwright install chromium
python Code/MingEngine/Editor/GenerateBuiltinIcons.py
```

4. 检查生成差异：SVG 与清单名称一致，PNG 背景透明，主次层与断笔在小尺寸下仍清晰。再次运行脚本应报告 `Unchanged`；浏览器或 Pillow 版本变化可能影响 PNG 编码，应核对差异。
5. 通过 VS Code 的 `Build MingEngine Debug` 任务重新构建，并在对应编辑器位置检查大小、对齐、主题染色和可辨认程度。仅新增源文件不会立即改变正在运行的编辑器。

生成流程与产物：

```text
EditorIcons/*.svg + manifest.json
  → Code/MingEngine/Editor/Icons/*.png
  → Code/MingEngine/Editor/BuiltinIcons.hpp

EditorIcons/assets/ming-glyph.png + ming-beam.png
  → Code/MingEngine/Editor/BuiltinBrand.hpp
```

修改 SVG 源文件和清单后统一运行生成器，不手改生成的 PNG 或头文件。生成器会删除输出目录中不在清单内的 PNG，因此新增图标必须先补充清单。脚本通过自身位置定位路径；引擎运行时使用内嵌数据，不依赖这个源目录，也不在运行时解析 SVG。

`assets` 中的两个品牌图片是从旧原型恢复的 2x RGBA 源贴图，分别表示“明”字和向右淡出的横笔。本次未恢复旧网页及品牌生成工具，现有脚本读取这两个 PNG 并将其内嵌，不会重新绘制品牌图片。

## 制作新图标时可复用的描述

> 为 Ming 编辑器制作「图标名称 / 含义」的 SVG 图标。参考本目录同族图标「参考文件名」，保持 16×16 viewBox、width="16" height="16" 和透明背景，以 Godot 同类概念的清晰轮廓为参考。显示路径使用 currentColor，颜色由主题控制。同族共用几何母题；主体清晰，补充信息可用 45% 不透明度淡层，与主层留约 1 单位透明断笔间隙；动作图标优先单层。轮廓紧凑，圆角按部件大小分级，描边使用圆头，不加入正文文字、渐变、阴影、纹理或背景；仅类型图标可使用转成路径的简短维度角标。输出可编辑的 SVG，并说明建议使用的 manifest family、tone 和 abstract。检查 16、20、24px 下是否能辨认。

若外部工具只生成 PNG，可将它作为草图参考；本目录的正式图标源文件仍需制作成 SVG，并遵守上述单色染色和透明度规则。
