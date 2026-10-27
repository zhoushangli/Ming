# Native field verification

Run from the repository root on Windows with Visual Studio 18:

```powershell
powershell -NoProfile -File Tests/RunNativeFieldsVerification.ps1
```

The runner builds the existing Debug solution task, links a verification executable against its native objects, and creates its fixtures under `Temporary/NativeFieldsVerification/Project`. Use `-SkipBuild` only after building the current Debug sources.

Coverage includes strict integer/float parsing, overflow and underflow, float display round trips, edit/commit/cancel/blur, range and step boundaries, button state, enum names and values, popup mouse/keyboard input, component preservation, Euler degrees, RGBA bytes, Vector2 JSON, resource type checks, and object deletion.

Integration checks exercise the production Window → EditorUI → Inspector path, setter readback, scene dirty state, save and uncached reload, plus Start Scene draft cancellation and restoration after a real filesystem write failure. GPU readback checks rendering clips and texture UVs, including flipped textures.

The production integration path now runs without an ImGui system or context. Tree collapse/filter/rename, native tabs, horizontal and vertical scrolling, viewport mesh picking and texture sampling, resize mapping, camera capture cancellation, native modal blocking, filesystem scene activation, and editor/scene lifetime separation are covered. Project settings tests exercise the retained form and persistence logic without entering its old ImGui renderer.

Rendered captures are written to `Temporary/NativeFieldsVerification/Captures/` as PPM files. They are real renderer output, not desktop screenshots.

Check the production App startup paths (after the field suite has generated its fixture):

```powershell
powershell -NoProfile -File Tests/RunNativeFieldsVerification.ps1 -SkipBuild -Suite NativeEditorStartupVerification
python Tests/ExportNativeEditorEvidence.py
```

The startup suite checks project manager, editor, and game modes in separate processes, including normal shutdown and the absence of an ImGui context. It stages the existing managed runtime files beside the test executable. Its test project contains no project C# assembly, so the engine may report that it continues without Game.dll.

The evidence exporter uses Pillow for PNGs and OpenCV when available for a checkpoint video. The video is an automated sequence of captured rendering checkpoints, not a desktop interaction recording.

Desktop visual acceptance remains separate: run `Run/MingEngine_Debug.exe --editor --project <test-project>` and inspect window resizing, text and error layout, dropdown placement, focus, scrolling, and the native Open Project dialog. Automated checks do not establish operating-system mouse, DPI, file-dialog, or desktop recording acceptance.
