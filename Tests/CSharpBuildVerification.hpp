#pragma once

#include "MingEngine/Scene/GUI/ProgressBar.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

inline int VerifyCSharpBuild(std::filesystem::path const& directory)
{
	std::filesystem::path const project = directory / ("Build-" + std::to_string(GetTickCount64()));
	int failures = 0;
	auto check = [&](bool success, char const* message)
	{
		std::printf("%s: %s\n", success ? "PASS" : "FAIL", message);
		std::fflush(stdout);
		if (!success)
		{
			++failures;
		}
	};
	std::filesystem::create_directories(project);
	std::ofstream(project / "project.ming")
		<< R"({"type":"ProjectSettings","version":1,"start_scene_path":"res://Main.tscn"})";
	std::ofstream(project / "Main.tscn") << R"({"nodes":[{"name":"BuildScene","type":"Node3D","parent":""}]})";
	MingRunConfig config;
	config.mode = MingRunMode::Editor;
	config.projectPath = project;
	App app(config);
	g_app = &app;
	app.Startup();
	auto* editor = EditorNode::Get();
	auto* scripts = g_engine->m_scriptSystem;
	auto* viewport = static_cast<Viewport*>(app.GetSceneTree()->GetRoot());
	auto closeWarning = [&]()
	{
		if (auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup()))
		{
			popup->Close();
		}
	};
	auto waitFor = [&](auto predicate)
	{
		auto const deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
		while (!predicate() && std::chrono::steady_clock::now() < deadline)
		{
			app.RunFrame();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		return predicate();
	};
	auto idle = [&]() { return !editor->IsBuildingCSharpProject() && !editor->IsPreparingPlay(); };

	// 1) Verify progress drawing independently of the build process
	auto* bar = MemNew<ProgressBar>();
	viewport->AddNode(bar);
	bar->SetThemeSource(editor->m_editorUI->GetRootControl());
	bar->SetPosition(Vector2(20.f, 120.f));
	bar->UpdateSize(Vector2(200.f, 12.f));
	auto background = CreateRef<ThemeStyleBox>();
	background->SetBackgroundColor(Color(20, 30, 40));
	auto fill = CreateRef<ThemeStyleBox>();
	fill->SetBackgroundColor(Color(200, 100, 50));
	bar->AddThemeStyleBoxOverride("background", background);
	bar->AddThemeStyleBoxOverride("fill", fill);
	check(bar->GetMinValue() == 0.f && bar->GetMaxValue() == 100.f && bar->GetValue() == 0.f
		&& !bar->IsIndeterminate(), "progress defaults match the public API");
	bar->SetValue(50.f);
	app.RunFrame();
	auto pixel = [&](unsigned x)
	{
		return ReadPixel(g_engine->m_renderServer->ViewportGetTexture(viewport->GetViewportRID()), x, 125);
	};
	check(pixel(40) == Color(200, 100, 50) && pixel(180) == Color(20, 30, 40),
		"determinate progress fills only its normalized range");
	bar->SetValue(200.f);
	check(bar->GetValue() == 100.f, "progress clamps values above the range");
	bar->SetValue(-10.f);
	check(bar->GetValue() == 0.f, "progress clamps values below the range");
	bar->SetMaxValue(0.f);
	app.RunFrame();
	check(pixel(40) == Color(20, 30, 40), "degenerate progress range draws an empty bar");
	bar->SetMinValue(10.f);
	app.RunFrame();
	check(pixel(40) == Color(20, 30, 40), "reversed progress range draws an empty bar");
	bar->SetIndeterminate(true);
	app.GetSceneTree()->UpdateScene(0.4f);
	g_engine->m_renderServer->Render();
	Color const first = pixel(50);
	app.GetSceneTree()->UpdateScene(0.4f);
	g_engine->m_renderServer->Render();
	check(first == Color(200, 100, 50) && pixel(50) == Color(20, 30, 40),
		"indeterminate progress moves with frame time");
	bar->SetIndeterminate(false);
	check(!bar->GetProcess(), "determinate progress does not keep processing");
	editor->SetEditorScale(1.f);
	float const baseHeight = bar->GetInternalMinimumSize().y;
	editor->SetEditorScale(2.f);
	check(bar->GetInternalMinimumSize().y == baseHeight * 2.f, "progress minimum height follows editor theme scaling");
	bar->AddThemeConstantOverride("minimum_height", 27);
	check(bar->GetInternalMinimumSize().y == 27.f, "progress observes live theme overrides");
	editor->SetEditorScale(1.25f);
	bar->QueueFree();
	app.RunFrame();

	// 2) Native projects run without generating or building a C# project
	editor->RequestPlay();
	check(waitFor([&]() { return app.IsGameRunning(); }) && !std::filesystem::exists(project / "Game.csproj"),
		"native-only Play skips C# generation and build");
	app.StopGame();
	check(waitFor([&]() { return !app.IsGameRunning(); }), "native game stops");
	check(scripts->EnsureProjectSolution(), "build checks prepare their own C# project");
	auto writeScript = [&](int value)
	{
		std::ofstream(project / "BuildProbe.cs") << "public static class BuildProbe { public const int Value = " << value
			<< "; public static Ming.ProgressBar Create() { var bar = new Ming.ProgressBar(); bar.Value = 25; "
			<< "bar.Indeterminate = true; return bar; } }";
	};
	writeScript(1);
	auto delayBuild = [&](int milliseconds)
	{
		std::ofstream(project / "BuildDelay.ps1") << "$PID | Set-Content -LiteralPath $PSScriptRoot/BuildChild.pid\n"
			<< "Start-Sleep -Milliseconds " << milliseconds << "\n";
		std::ofstream(project / "Directory.Build.targets") << R"XML(<Project><Target Name="DelayVerificationBuild" BeforeTargets="BeforeBuild"><Exec Command="powershell -NoProfile -ExecutionPolicy Bypass -File &quot;$(MSBuildProjectDirectory)/BuildDelay.ps1&quot;" /></Target></Project>)XML";
	};
	delayBuild(1200);
	auto const started = std::chrono::steady_clock::now();
	check(editor->BuildCSharpProject(), "menu build starts asynchronously");
	check(!editor->m_editorUI->IsBuildProgressVisible(), "progress popup stays hidden at build start");
	VirtualPath const scenePath = editor->GetCurrentScenePath();
	editor->RequestNewScene();
	editor->RequestLoadScene("res://Other.tscn");
	editor->RequestPlay();
	check(!editor->BuildCSharpProject() && !editor->ReloadCSharpAssembly()
		&& editor->GetCurrentScenePath() == scenePath && !editor->IsPreparingPlay(),
		"build rejects duplicate builds, Play, reload and scene changes");
	int frames = 0;
	bool seenPopup = false;
	bool earlyPopup = false;
	bool popupCloses = false;
	bool closeButton = false;
	auto const deadline = started + std::chrono::seconds(60);
	while (!idle() && std::chrono::steady_clock::now() < deadline)
	{
		app.RunFrame();
		++frames;
		if (editor->m_editorUI->IsBuildProgressVisible())
		{
			seenPopup = true;
			earlyPopup = earlyPopup || std::chrono::steady_clock::now() - started < std::chrono::milliseconds(250);
			auto* popup = viewport->GetGuiPopup();
			closeButton = closeButton || FindScriptControl<Button>(popup,
				[](Button* button) { return button->GetName() == String("CloseDialog"); }) != nullptr;
			InputEvent key;
			key.m_type = InputEventType::Key;
			key.m_pressed = true;
			key.m_keyCode = GLFW_KEY_ESCAPE;
			popup->GuiInput(key);
			popupCloses = popupCloses || !editor->m_editorUI->IsBuildProgressVisible();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	check(idle() && editor->GetCSharpBuildExitCode() == 0 && !app.IsGameRunning(),
		"first menu build succeeds without launching the game");
	check(frames > 10 && seenPopup && !earlyPopup && !popupCloses && !closeButton
		&& !editor->m_editorUI->IsBuildProgressVisible(), "slow build keeps frames advancing and uses a delayed nonclosable popup");

	// 3) Let MSBuild decide which compilation outputs can be reused
	std::filesystem::remove(project / "Directory.Build.targets");
	check(editor->BuildCSharpProject() && waitFor(idle) && editor->GetCSharpBuildExitCode() == 0,
		"build succeeds after changing imported build configuration");
	auto const assembly = project / ".ming/dotnet/bin/Debug/Game.dll";
	auto const unchangedTime = std::filesystem::last_write_time(assembly);
	check(editor->BuildCSharpProject() && waitFor(idle) && editor->GetCSharpBuildExitCode() == 0
		&& std::filesystem::last_write_time(assembly) == unchangedTime, "unchanged build reuses the existing DLL");
	auto* scene = static_cast<Node3D*>(app.GetSceneTree()->GetScene());
	scene->SetLocalPosition(Vector3(1.f, 2.f, 3.f));
	editor->MarkSceneDirty();
	check(editor->SaveScene(), "save a scene-only transform change");
	check(editor->BuildCSharpProject() && waitFor(idle) && editor->GetCSharpBuildExitCode() == 0
		&& std::filesystem::last_write_time(assembly) == unchangedTime, "scene-only edits reuse the C# DLL");
	writeScript(2);
	check(editor->BuildCSharpProject() && waitFor(idle) && editor->GetCSharpBuildExitCode() == 0
		&& std::filesystem::last_write_time(assembly) != unchangedTime, "script edits update the DLL");
	std::ofstream(project / "BuildProbe.cs") << "syntax error";
	editor->RequestPlay();
	check(waitFor(idle) && editor->GetCSharpBuildExitCode() != 0 && !app.IsGameRunning()
		&& !editor->m_editorUI->IsBuildProgressVisible(), "compiler errors block Play and dismiss progress");
	closeWarning();
	writeScript(2);
	std::wstring const oldPath = _wgetenv(L"PATH") != nullptr ? _wgetenv(L"PATH") : L"";
	_wputenv_s(L"PATH", project.c_str());
	bool const missingDotnetRejected = !editor->BuildCSharpProject();
	_wputenv_s(L"PATH", oldPath.c_str());
	check(missingDotnetRejected && idle() && editor->GetCSharpBuildExitCode() != 0,
		"missing dotnet fails cleanly without leaving a pending build");
	closeWarning();
	std::ofstream(project / "Directory.Build.targets") << R"XML(<Project><Target Name="DeleteVerificationAssembly" AfterTargets="Build"><Delete Files="$(TargetPath)" /></Target></Project>)XML";
	check(editor->BuildCSharpProject() && waitFor(idle) && editor->GetCSharpBuildExitCode() == 0
		&& FindScriptControl<Label>(viewport->GetGuiPopup(), [](Label* label)
			{ return label->GetText().ToUtf8().find("Build failed.") != std::string::npos; }) != nullptr,
		"zero build exit code with missing Game.dll is a failure");
	closeWarning();
	check(!scripts->BuildProjectSolution(), "synchronous build also rejects a missing output assembly");

	// 4) Quit while a child process is active and verify it is reclaimed
	std::filesystem::remove(project / "BuildChild.pid");
	delayBuild(20000);
	editor->RequestPlay();
	check(waitFor([&]() { return std::filesystem::exists(project / "BuildChild.pid"); }), "shutdown fixture starts a build child");
	DWORD childPid = 0;
	std::ifstream(project / "BuildChild.pid") >> childPid;
	HANDLE const child = childPid != 0 ? OpenProcess(SYNCHRONIZE, FALSE, childPid) : nullptr;
	editor->RequestQuit();
	check(app.IsQuitting() && idle() && !app.IsGameRunning() && !editor->m_editorUI->IsBuildProgressVisible()
		&& child != nullptr && WaitForSingleObject(child, 2000) == WAIT_OBJECT_0,
		"quit terminates the build child and discards pending Play");
	if (child != nullptr)
	{
		CloseHandle(child);
	}
	app.Shutdown();
	g_app = nullptr;
	std::filesystem::remove(project / "Directory.Build.targets");
	std::printf("CSHARP_BUILD failures=%d\n", failures);
	return failures == 0 ? 0 : 1;
}
