# Native field verification

## Editor dimensions, MenuBar and split cursors

After the current Debug build, run `powershell -NoProfile -File Tests/RunNativeFieldsVerification.ps1 -SkipBuild -Suite EditorUIVerification`.
This independent suite covers menu hover/keyboard switching, command activation, popup cleanup, split cursor capture/cancellation, editor/runtime theme isolation, and Tree focus appearance. Renderer captures at 1440x900 and 960x600 are written under `Temporary/EditorUIVerification/Captures/`; native cursor appearance and desktop interaction still require a desktop check.

`EditorThemeManager::CreateTheme()` stores final editor font sizes, dimensions and StyleBox margins directly. `Editor/top_bar_height` controls the shell header; `MenuBar/minimum_height` controls menu title height. Editor Tree focus borders are transparent; keyboard focus, selection and existing text/scroll layout remain available.

## Text layout and themed container dimensions

After the current Debug build, run the independent local suite:

```powershell
powershell -NoProfile -File Tests/RunNativeFieldsVerification.ps1 -SkipBuild -Suite TextLayoutVerification
```

Open the interactive specimen directly, without running the other verification suites:

```powershell
powershell -NoProfile -File Tests/RunNativeFieldsVerification.ps1 -SkipBuild -Suite TextLayoutVerification -Preview
```

Resize the window to compare clipping, character trimming, ellipsis, and wrapped paragraph height. **Update Theme** changes font size, line spacing, sibling separation, and outer margins; **Close preview** exits. The runner links against current Debug objects; `-SkipBuild` skips the engine build, not the small verification executable build. The executable also accepts `<project-directory> --preview` directly from the `Run` working directory.

`Label` supports `SetClipText`, `SetTextOverrunBehavior`, and `SetAutowrapMode`; `Button` supports the first two and stays single-line. `TrimNothing`, `TrimCharacters`, and `EllipsisAlways` retain the original source text; ellipsis appears only when overflowing, uses U+2026, and falls back to three dots when absent from the font. Text clipping does not change `Control::SetClipContents`. Enabled clipping/trimming relaxes natural minimum width; explicit custom minimum sizes remain in force. Wrapped Labels keep the full content height even with clipping enabled.

`WordSmart` supports ordinary UTF-32 text: English word boundaries, character splitting for long words, CJK character boundaries, CR/LF normalization, and explicit blank lines. Nonpositive width means width has not been assigned yet. Complex shaping, grapheme clusters, complete Unicode line-breaking rules, font fallback, maximum line counts, and last-line ellipsis are outside this slice. Font glyph coverage remains the caller's responsibility.

New Theme items are `Label/line_spacing`, `BoxContainer/separation`, and `MarginContainer/margin_left`, `margin_top`, `margin_right`, `margin_bottom`; each defaults to zero. Label line height includes font ascent/descent and, between lines only, nonnegative font line gap plus additional Theme line spacing. Existing dimension setters remain floating-point instance overrides. `RemoveSeparationOverride()` and `RemoveMarginsOverride()` restore Theme values. A single-edge margin setter changes only that edge. StyleBox content margins inset its contents, container separation spaces siblings, and MarginContainer adds explicit outer padding; avoid applying the same padding at both levels.

The suite prepares its own font, Theme, nested layout, and synthetic rectangle-glyph font with kerning and no ellipsis. It checks resize convergence, live Theme changes, float overrides, hidden children, icon space reservation, and GPU glyph clipping, writing `Temporary/TextLayoutVerification/Captures/text-layout-clipping.ppm`. This is renderer evidence; interactive desktop acceptance is reported separately.

## Existing native fields suite

Run from the repository root on Windows with Visual Studio 18:

```powershell
powershell -NoProfile -File Tests/RunNativeFieldsVerification.ps1
```

The runner builds the existing Debug solution task, links a verification executable against its native objects, and creates its fixtures under `Temporary/NativeFieldsVerification/Project`. Use `-SkipBuild` only after building the current Debug sources.

Coverage includes strict integer/float parsing, overflow and underflow, float display round trips, edit/commit/cancel/blur, range and step boundaries, button state, enum names and values, popup mouse/keyboard input, component preservation, Euler degrees, RGBA bytes, Vector2 JSON, resource type checks, and object deletion.

Integration checks exercise the production Window → EditorUI → Inspector path, setter readback, scene dirty state, save and uncached reload, plus Start Scene draft cancellation and restoration after a real filesystem write failure. GPU readback checks rendering clips and texture UVs, including flipped textures.

The production integration path uses native controls; the ImGui subsystem, rendering adapters, third-party sources, and build dependencies have been removed. Tree collapse/filter/rename, native tabs, horizontal and vertical scrolling, viewport mesh picking and texture sampling, resize mapping, camera capture cancellation, native modal blocking, filesystem scene activation, and editor/scene lifetime separation are covered. Project settings tests exercise the native modal, draft cancellation, keyboard focus, validation, and persistence.

Drag/Dock checks cover the eight-unit threshold, capture handover, parent acceptance and mouse filters, clipping, preview isolation, commit-once behavior, Escape/focus/modal/source-deletion cancellation, and ordinary clicks when a source declines dragging. Editor checks cover node reparenting and world position, resource class compatibility and value callbacks, file movement with import metadata and rejection rules, delayed tab switching while preserving a hidden source, pointer-driven panel transfer, tab identity after reordering, divider bounds, layout reload in a new editor, malformed/partial configuration, and recovery from a layout write failure.

Editor layouts are stored in `res://.ming/Editor/NativeEditorLayout.json`. Split indexes exposed by the editor are 0: left/body, 1: center/right, 2: left upper/lower, and 3: viewport/output. Dock slot indexes are 0: left upper (Scene), 1: left lower (FileSystem), 2: right (Inspector/Import), and 3: below the viewport (Output).

Rendered captures are written to `Temporary/NativeFieldsVerification/Captures/` as PPM files. They are real renderer output, not desktop screenshots.

Theme checks cover explicit `ThemeCache` bindings, inherited fields, local overrides, variation fallback, default font/size, ordered Context fallback, nearest-scope isolation, detached popup sources, source replacement/deletion, resource moves, and subscription cleanup. Button draws text and icons directly; there is no internal Label. `theme-ink-states.ppm`, `theme-ink-updated.ppm`, and `theme-ink-layout-updated.ppm` capture the small Warm Carbon Ink specimen before and after live edits.

`EditorThemeManager::CreateTheme()` assembles Warm Carbon for the editor, including 15px body text, 30px minimum fields/rows, compact tabs, toolbar and primary-button variations, Inspector headings, errors, and existing icons. `EditorNode` installs it in the editor Context. The scene SubViewport has its own empty Context and falls back to the independent neutral runtime Theme, whose body size is 16px. ThemeDatabase stores the runtime baseline and Context lists; Control performs lookup and refresh. Theme lookup is override, local Control ancestry, nearest Context sources in order, runtime default, then typed fallback. Font and size defaults apply after all matching types in each individual Theme. A nearer Context replaces an outer Context; it does not concatenate their sources.

`EditorPanelsVerification.hpp` prepares its own scene and OBJ fixture under `Project/PanelMigration`. It exercises native menus, settings cancellation and modal Tab focus, folder creation/rename/duplicate/delete confirmation, scene and node creation, unsaved-scene/quit cancellation, import options and reimport persistence, and panel close/reopen. `editor-native-panels.ppm` is renderer output from that flow. The legacy editor renderer, EditorControlHost, immediate-mode property adapters, and recursive font application have been removed. Panels are Control nodes owned by the scene tree; EditorUI keeps borrowed pointers and handles shell layout and docking. The engine has no optional ImGui path.

Only layout version 2 is accepted. Old or malformed caches are replaced by the Warm Carbon default; legacy EditorLayout.ini is deleted on project open. Panel visibility, selected tabs and split ratios persist in the new cache. Output shows its existing startup message; unimplemented menu and viewport actions remain disabled. Pause/Stop and Profiler are outside this migration.

To leave the specimen open for desktop inspection after the automated checks, run:

```powershell
$env:MING_THEME_PREVIEW = '1'
try {
    powershell -NoProfile -File Tests/RunNativeFieldsVerification.ps1 -SkipBuild
} finally {
    Remove-Item Env:MING_THEME_PREVIEW
}
```

The preview displays editable/read-only/disabled fields, a renamable tree inside a scrolling pane, dedicated tab styles, another dual-axis scroll page, and both splitter orientations. **Update theme** toggles font size, row height/indentation, tab height, scrollbar/divider thickness, and mutable StyleBox colors/margins. **Close preview** finishes the suite. Use F2 to rename a selected row, Enter to commit, Escape to cancel, and switch tabs or drag scrollbars/dividers. Window resizing relayouts the specimen.

`theme-controls-compact.ppm` and `theme-controls-expanded.ppm` capture the combined specimen. The expanded capture is taken during an active rename and checks that its selection survives. The original Button-only specimen remains available with `MING_BUTTON_THEME_PREVIEW=1`.

Theme cache fields and bindings use the exact theme item name, for example `BIND_THEME_ITEM(Theme::DataType::Color, LineEdit, caret_color)`. Important item groups:

| Control | Theme item groups |
| --- | --- |
| Button | `font`, `font_size`, `icon`, `icon_size`, `icon_separation`, state styles/colors, `minimum_height` |
| LineEdit | `font`, `font_size`, `minimum_height`, `normal`, `hover`, `read_only`, `disabled`, `focus`, `font_*_color`, `selection_color`, `caret_color`, `caret_width` |
| Tree | `font`, `font_size`, `row_height`, `indent`, `icon_size`, `icon_separation`, per-item icons, `panel` content margins, `hover`, `selected`, `focus`, `drop_mark`, `font_color`, `font_selected_color` |
| TabContainer | `font`, `font_size`, `tab_height`, `tab_minimum_width`, `tab_separation`, `page_separation`, `tab_*` styles, state font colors, `panel`, `drop_mark_color`, `drop_mark_width` |
| ScrollContainer | `background_color`, `track`, `grabber`, `grabber_hover`, `grabber_pressed`, `scrollbar_width`, `grabber_minimum_size`, `grabber_margin`, `scroll_step` |
| SplitContainer | `separation`, `normal`, `hover`, `pressed`, `grabber_length`, `grabber_width`, `grabber_separation`, and grabber state colors |

LineEdit takes per-edge maximum content margins across its normal/hover/read-only/disabled StyleBoxes; focus is drawn as an overlay. `SetPadding(Vector2)` provides a symmetric floating-point override; `RemovePaddingOverride()` restores the themed margins. `GetPadding()` returns their horizontal/vertical maxima, while `GetContentRect()` exposes the actual asymmetric content region. Font setters use standard theme overrides and can be cleared with the existing remove-override APIs.

Tree exposes `GetRowHeight()` and `GetItemRect(id)`; TabContainer exposes `GetTabBarRect()`, `GetPageRect()`, and `GetTabInsertRect(index)`; SplitContainer exposes `GetDividerRect()`. These are local-space geometry queries shared with drawing and input. Unknown/hidden tree items and absent dividers return an empty rectangle.

Live theme updates preserve LineEdit/Tree rename text, selection, cursor, and undo history. Geometry changes cancel an active scrollbar/divider drag without invoking its completion callback; existing release/cancellation handles mouse capture. Pure color changes keep interaction state. Tests cover both compact and expanded dimensions, clipping/hits, row reconstruction notifications, inherited/overridden values, both scroll axes, and divider minimums.

Check the production App startup paths independently:

```powershell
powershell -NoProfile -File Tests/RunNativeFieldsVerification.ps1 -SkipBuild -Suite NativeEditorStartupVerification
```

The startup suite prepares its own minimal scene under `Temporary/NativeEditorStartupVerification/Project/Startup` and checks project manager, editor, and game modes in separate processes, including normal shutdown. The editor additionally writes warm-carbon-1440x900.ppm and warm-carbon-960x600.ppm with a selected real node; these are renderer captures, not desktop acceptance. It stages the existing managed runtime files beside the test executable. Its test project contains no project C# assembly, so the engine may report that it continues without Game.dll.

The evidence exporter uses Pillow for PNGs and OpenCV when available for a checkpoint video. The video is an automated sequence of captured rendering checkpoints, not a desktop interaction recording.

Desktop visual acceptance remains separate: run `Run/MingEngine_Debug.exe --editor --project <test-project>` and inspect window resizing, text and error layout, dropdown placement, focus, scrolling, and the native Open Project dialog. Automated checks do not establish operating-system mouse, DPI, file-dialog, or desktop recording acceptance.

The Drag/Dock desktop session on 2026-10-29 was stopped by the user with Escape before completion. It identified a queued mouse-button position issue; the callback now uses the last dispatched motion position. Real mouse dragging after that correction, divider interaction, restart restoration, and DPI behavior still require desktop acceptance. The automated checks above are separate evidence.

## Warm Carbon icon generation

`python Code/MingEngine/Editor/GenerateBuiltinIcons.py` uses the 254 SVGs and manifest in `EditorStyleTest/warm-carbon/icons` as its only source. Python Pillow, Playwright and an installed Playwright Chromium are required for generation only. Chromium preserves SVG masks and opacity; the output is 32x32 white RGBA PNG for 16 logical pixel display. The script removes PNGs absent from the manifest, embeds all icons and their tone metadata in `Editor/BuiltinIcons.hpp`, and embeds the two original brand PNGs in `Editor/BuiltinBrand.hpp`. The executable has no browser or prototype-directory dependency.

Editor tree items carry their own icon color; the theme resolves neutral, 2D, 3D, GUI and status colors. Selection changes the row background, preserving the icon type color. Icons absent for a class resolve through registered ancestors and then the new Node icon. The native panel suite checks all embedded textures, default regions, old-cache invalidation and non-mutating Inspector filtering.

The editor loads its font with em sizing so theme font sizes follow CSS-style em sizing. Other font loads retain the existing total-height convention; TextLayoutVerification checks both contracts. EditorPanel includes its child layout's minimum size, keeping toolbars and scrollbar hit areas inside the panel when narrowed.
