# 自研编辑器 UI 阶段

本阶段提供固定分区的自研编辑器、资源目录浏览、场景打开、节点选择与重命名、反射属性编辑，以及保存重开。所有 App 启动模式均关闭 ImGui 的运行配置；第三方源码、编译依赖和未迁移的旧界面仍保留。

## 实现参考

参考源码位于 `C:/Work/Godot/Editor/godot/scene/gui/`。采用 Ming 现有通知、回调、对象标识和 Canvas 渲染接口进行实现，不直接引入 Godot 的对象系统或主题系统。

| Godot 参考 | Ming 中采用的行为 |
| --- | --- |
| `tree.h`、`tree.cpp` 中的选择、折叠和编辑状态 | 独立条目标识、可见行、单选、键盘导航、LineEdit 重命名 |
| `tab_container.cpp` 的页面显隐与布局 | 隐藏非当前页面，清理旧页编辑与捕获，分配当前页内容区域 |
| `scroll_container.cpp` 的内容最小尺寸与滚动范围 | 双向滚动条互相占位、范围收缩、自动显隐、焦点项滚入可见区 |
| `subviewport_container.cpp` 的尺寸同步与纹理显示 | 场景输出分辨率跟随显示区域，显示时解析视口 RID，转换鼠标位置 |
| `popup.cpp`、`popup_panel.cpp` 的弹出层职责 | 模态输入阻断、独立弹出内容区域、确认和取消 |

Tree 的业务数据映射留在编辑器面板侧；Tree 本身不认识场景节点或文件路径。第一版不提供多列、多选、虚拟化和拖放。TabContainer 内部复用 Button，没有额外公开 TabBar。

## 布局与生命周期

根 Window 显示编辑器 UI。中间 SubViewportContainer 持有场景 SubViewport；场景与编辑器相机挂在该视口下。SceneTree 的默认场景挂载点仍为根视口，仅编辑器显式改变挂载点。

编辑器 UI 由 EditorNode 持有，与被编辑场景分开。保存只打包当前场景根节点。场景打开采用未缓存读取，确保保存后重新打开能读到磁盘上的新内容。

Canvas 命令保存源视口 RID，不保存可因 resize 失效的 GPUTexture 指针。RenderServer 在消费者之前渲染其引用的视口，并拒绝直接采样自身输出。

字体由现有内嵌 JetBrains Mono 数据解压后作为独立字节数组交给 TextServer。新运行路径不读取 ImGui 的字体图集。字体字符覆盖范围沿用现有字体，中文名称的完整字形覆盖需要另行配置支持中文的字体。

## 操作与暂缓范围

- 无参数启动项目管理器，使用 Open Project 选择 `project.ming`。
- 编辑器左侧 Scene/FileSystem 标签切换；两页均支持名称过滤和双向滚动。
- Scene 支持点击、方向键选择、展开折叠和 F2 重命名。重命名 Enter 提交、Escape 取消、失焦提交。
- FileSystem 双击或 Enter 打开 `.tscn` 场景；Refresh 重新扫描资源目录。
- 工具栏 Open Scene 支持输入 `res://` 场景路径；Save 或 Ctrl+S 提交编辑并保存。
- 在场景区域右键进入相机导航；移出区域释放右键、窗口失焦或交互取消时结束捕获。左键拾取使用现有网格射线检测。
- 修改场景后切换或退出会显示原生确认弹窗。保存失败保留当前场景；取消不会丢弃修改。
- Import、项目设置、节点创建与删除、文件写操作、Gizmo 拖动和通用拖放没有接入新界面。旧代码与业务逻辑保留。
- 分区宽度固定，窗口伸缩时中间区域变化；暂不保存编辑器布局和运行时焦点、悬停、捕获状态。

## 验证与证据

构建使用 `.vscode/tasks.json` 的 Debug/Release 任务。自动验证和生成材料的命令见 `Tests/README.md`。

字段与编辑器验证生成一个含真实网格的测试项目，检查属性输入、节点重命名、场景拾取、视口采样、过滤、滚动、模态阻断、保存和重开。启动验证另外检查项目管理器、编辑器和游戏模式。

`Temporary/NativeFieldsVerification/Captures/` 中的 PNG 来自真实 GPU 渲染输出。`rendered-checkpoints.mp4` 是自动测试检查点的演示视频，不是操作系统桌面录屏。桌面鼠标操作、DPI、Windows 文件选择器和完整人工操作录屏需要单独验收。

论文可表述为：**基于 Godot 控件设计参考，实现自研编辑器 UI，并接通基础场景编辑流程；ImGui 保留于源码与构建中，运行路径已停用。**
