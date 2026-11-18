#define NOMINMAX

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectManagerState.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <cmath>
#include <limits>

#include "MingEngine/Engine/Application/Main_Windows.cpp"

namespace
{
int checks = 0;

void Check(bool passed, char const* message)
{
	++checks;
	if (!passed)
	{
		throw std::runtime_error(message);
	}
}

void Write(std::filesystem::path const& path, char const* text)
{
	std::ofstream output(path);
	output << text;
	Check(output.good(), "write fixture");
}

void VerifyArguments(std::filesystem::path const& project)
{
	std::wstring executable = L"verification.exe";
	std::wstring projectPath = project.wstring();
	std::wstring projectFlag = L"--project";
	std::wstring editor = L"--editor";
	std::wstring game = L"--game";
	wchar_t* args[] = { executable.data(), editor.data(), projectFlag.data(), projectPath.data(), game.data() };
	MingRunConfig config;
	Check(TryParseRunConfig(config, 1, args) && config.mode == MingRunMode::ProjectManager, "default mode");
	Check(!TryParseRunConfig(config, 2, args), "editor requires project");
	Check(TryParseRunConfig(config, 4, args) && config.mode == MingRunMode::Editor, "editor arguments");
	Check(!TryParseRunConfig(config, 5, args), "reject conflicting modes");
	args[1] = game.data();
	Check(TryParseRunConfig(config, 4, args) && config.mode == MingRunMode::Game, "game arguments");
}

void VerifyScenes(App& app)
{
	SceneSystem* scenes = app.GetEngine()->m_sceneSystem;
	std::string error;
	ObjectID const initial = scenes->GetCurrentScene()->GetObjectID();
	Check(!scenes->ChangeScene(VirtualPath("res://missing.tscn"), error), "missing scene fails");
	Check(scenes->GetCurrentScene()->GetObjectID() == initial, "failed load preserves scene");
	Node* autoload = scenes->GetSceneTree()->GetRoot()->GetNode(NodePath("Session"));
	Check(autoload != nullptr, "autoload exists");
	ObjectID const autoloadID = autoload->GetObjectID();
	Check(scenes->ChangeScene(VirtualPath("res://other.tscn"), error), "queue scene change");
	scenes->EndFrame();
	Check(scenes->GetCurrentScene()->GetName() == "Other", "commit scene change");
	Check(ObjectDatabase::GetInstance<Node>(initial) == nullptr, "old scene destroyed");
	Check(ObjectDatabase::GetInstance<Node>(autoloadID) == autoload, "autoload survives scene change");
	ObjectID const beforeReload = scenes->GetCurrentScene()->GetObjectID();
	Check(scenes->ReloadScene(error), "reload scene");
	scenes->EndFrame();
	Check(ObjectDatabase::GetInstance<Node>(beforeReload) == nullptr, "reload destroys old instance");
	Check(scenes->GetCurrentScene()->GetName() == "Other", "reload restores scene");

	Check(scenes->ChangeScene(VirtualPath("res://main.tscn"), error), "queue pending scene");
	scenes->ClearScene();
	scenes->EndFrame();
	Check(scenes->GetCurrentScene() == nullptr, "clear cancels pending replacement");
	Check(!scenes->ReloadScene(error), "cleared scene has no reload path");
	Node* detached = MemNew<Node>();
	Check(scenes->ChangeScene(detached, error), "detached scene change");
	scenes->EndFrame();
	Check(!scenes->ReloadScene(error), "detached scene has no reload path");
	scenes->ClearAutoloads();
	scenes->EndFrame();
	Check(ObjectDatabase::GetInstance<Node>(autoloadID) == nullptr, "clear autoloads");
	Check(!scenes->CreateAutoloads({ { "Valid", VirtualPath("res://autoload.tscn") },
		{ "Missing", VirtualPath("res://missing.tscn") } }, error), "partial autoload preparation fails");
	Check(scenes->GetSceneTree()->GetRoot()->GetNode(NodePath("Valid")) == nullptr, "failed autoload set stays detached");
	Check(scenes->CreateAutoloads({ { "Session", VirtualPath("res://autoload.tscn") } }, error), "autoloads can be retried");
	ObjectID const oldRoot = scenes->GetSceneTree()->GetRoot()->GetObjectID();
	scenes->Reset();
	Check(ObjectDatabase::GetInstance<Node>(oldRoot) == nullptr && scenes->GetCurrentScene() == nullptr, "reset destroys entire old tree");
}

class PhysicsProbe : public Node
{
public:
	void _EnterTree() override
	{
		++entered;
	}

	void _Ready() override
	{
		++ready;
	}

	void _Process(float deltaSeconds) override
	{
		++frames;
		validDeltas = validDeltas && deltaSeconds == GetSceneTree()->GetDeltaSeconds();
	}

	void _PhysicsProcess(float deltaSeconds) override
	{
		++ticks;
		simulated += deltaSeconds;
		validDeltas = validDeltas && std::abs(deltaSeconds - 1.f / 60.f) < 1e-7f
			&& deltaSeconds == GetSceneTree()->GetPhysicsDeltaSeconds();
	}

	void _ExitTree() override
	{
		*exitWithWorld = g_engine->m_physicsServer->Step(1.f / 60.f);
	}

public:
	int entered = 0;
	int ready = 0;
	int frames = 0;
	int ticks = 0;
	double simulated = 0.0;
	bool validDeltas = true;
	bool* exitWithWorld = nullptr;
};

void VerifyPhysics(MingRunConfig const& config)
{
	App app(config);
	app.Startup();
	Check(!app.IsQuitting(), "physics game startup");
	glfwHideWindow(app.GetEngine()->m_windowSystem->GetGLFWWindow());
	Engine* engine = app.GetEngine();
	SceneSystem* scenes = engine->m_sceneSystem;
	bool exitedWithWorld = false;

	for (int fps : { 30, 60, 120 })
	{
		auto probe = MemNew<PhysicsProbe>();
		probe->exitWithWorld = &exitedWithWorld;
		scenes->GetSceneTree()->GetRoot()->AddNode(probe);
		Check(probe->entered == 1 && probe->ready == 1, "renamed native lifecycle callbacks");

		for (int frame = 0; frame < fps * 10; ++frame)
		{
			engine->Update(1.f / fps);
		}

		Check(std::abs(probe->ticks - 600) <= 1, "render FPS independent fixed tick count");
		Check(std::abs(probe->simulated - 10.0) < 0.017, "render FPS independent simulation time");
		Check(probe->frames == fps * 10, "one ordinary callback per frame");
		Check(probe->validDeltas, "fixed and ordinary deltas remain separate");
		int before = probe->ticks;
		engine->Update(1.005f);
		Check(probe->ticks - before == 8, "stall capped at eight steps");
		before = probe->ticks;
		engine->Update(0.f);
		Check(probe->ticks == before, "whole backlog discarded");
		engine->Update(0.012f);
		Check(probe->ticks == before + 1, "fractional stall remainder retained");
		before = probe->ticks;
		engine->Update(-1.f);
		engine->Update(std::numeric_limits<float>::infinity());
		engine->Update(std::numeric_limits<float>::quiet_NaN());
		Check(probe->ticks == before, "invalid delta ignored");
		probe->SetPhysicsProcess(false);
		engine->Update(1.f / 60.f);
		Check(!probe->GetPhysicsProcess() && probe->ticks == before, "independent physics processing switch");
		probe->SetPhysicsProcess(true);
		ObjectID const probeID = probe->GetObjectID();
		exitedWithWorld = false;
		probe->QueueFree();
		engine->Update(1.f / 60.f);
		Check(ObjectDatabase::GetInstance<Node>(probeID) == nullptr && exitedWithWorld, "pending deletion before simulation with live world");
	}

	auto shutdownProbe = MemNew<PhysicsProbe>();
	exitedWithWorld = false;
	shutdownProbe->exitWithWorld = &exitedWithWorld;
	scenes->GetSceneTree()->GetRoot()->AddNode(shutdownProbe);
	app.Quit();
	engine->Update(1.f, [&app]() { return app.IsQuitting(); });
	Check(shutdownProbe->ticks == 0, "quit request stops fixed callbacks");
	app.Shutdown();
	Check(exitedWithWorld, "scene exits with an active physics world");
	Check(!engine->m_physicsServer->Step(1.f / 60.f), "shutdown prevents stepping");
}

void VerifyManaged(MingRunConfig const& config, std::filesystem::path const& project)
{
	auto log = project / "callbacks.log";
	std::filesystem::remove(log);
	_wputenv_s(L"MING_VERIFY_LIFECYCLE_LOG", log.c_str());
	App app(config);
	app.Startup();
	Check(!app.IsQuitting(), "managed lifecycle game startup");
	glfwHideWindow(app.GetEngine()->m_windowSystem->GetGLFWWindow());
	SceneSystem* scenes = app.GetEngine()->m_sceneSystem;
	Check(app.GetEngine()->Update(1.f / 60.f), "managed frame update");
	scenes->ClearScene();
	scenes->EndFrame();
	app.Shutdown();
	std::ifstream input(log);
	std::string actual;
	input >> actual;
	Check(actual == "ERFPX", "renamed managed lifecycle and physics dispatch");
	_wputenv_s(L"MING_VERIFY_LIFECYCLE_LOG", L"");
}

void VerifyMode(MingRunConfig const& config)
{
	App app(config);
	app.Startup();
	Check(!app.IsQuitting() && app.GetExitCode() == 0, "mode startup");
	Check(config.mode == MingRunMode::Game || !app.GetEngine()->m_physicsServer->Step(1.f / 60.f), "non-game mode has no active physics world");
	glfwHideWindow(app.GetEngine()->m_windowSystem->GetGLFWWindow());
	app.RunFrame();
	if (config.mode == MingRunMode::Game)
	{
		Check(EditorNode::Get() == nullptr, "game contains no editor");
		VerifyScenes(app);
	}
	else if (config.mode == MingRunMode::Editor)
	{
		Check(EditorNode::Get() != nullptr, "editor node exists");
		for (int run = 0; run < 2; ++run)
		{
			Check(app.LaunchGame(), "launch child game");
			Check(app.IsGameRunning(), "track child game");
			app.StopGame();
			for (int frame = 0; frame < 600 && app.IsGameRunning(); ++frame)
			{
				app.RunFrame();
			}
			Check(!app.IsGameRunning(), "stop and reap child game");
		}
		_wputenv_s(L"MING_VERIFY_CHILD_EXIT", L"1");
		Check(app.LaunchGame(), "launch self-exiting game");
		_wputenv_s(L"MING_VERIFY_CHILD_EXIT", L"");
		for (int frame = 0; frame < 600 && app.IsGameRunning(); ++frame)
		{
			app.RunFrame();
		}
		Check(!app.IsGameRunning(), "reap natural game exit");
	}

	if (config.mode == MingRunMode::Editor)
	{
		Check(app.LaunchGame(), "launch before editor shutdown");
	}
	app.Shutdown();
	app.Shutdown();
	Check(app.GetEngine()->m_sceneSystem->GetSceneTree() == nullptr && !app.IsGameRunning(), "idempotent shutdown");
	Check(!app.GetEngine()->m_physicsServer->Step(1.f / 60.f), "app shutdown closes physics world");
}
}

int wmain(int argc, wchar_t** argv)
{
	if (argc > 1 && std::wstring(argv[1]).find(L"--verify-") != 0)
	{
		MingRunConfig config;
		if (!TryParseRunConfig(config, argc, argv))
		{
			return 2;
		}
		App child(config);
		child.Startup();
		glfwHideWindow(child.GetEngine()->m_windowSystem->GetGLFWWindow());
		if (GetEnvironmentVariableW(L"MING_VERIFY_CHILD_EXIT", nullptr, 0) > 0)
		{
			child.RunFrame();
			child.Quit();
		}
		child.RunMainLoop();
		child.Shutdown();
		return child.GetExitCode();
	}

	try
	{
		std::wstring const module = argc > 1 ? argv[1] : L"--verify-arguments";
		auto project = std::filesystem::absolute(module == L"--verify-managed"
			? "../Temporary/ApplicationLifecycleVerification/ManagedProject"
			: "../Temporary/ApplicationLifecycleVerification/Project With Spaces");
		std::filesystem::create_directories(project / "Settings");
		_wputenv_s(L"LOCALAPPDATA", (project / "Settings").c_str());
		Write(project / "project.ming", R"({"type":"ProjectSettings","version":1,"start_scene_path":"res://main.tscn","autoloads":[{"name":"Session","path":"res://autoload.tscn"}]})");
		Write(project / "main.tscn", R"({"nodes":[{"name":"Main","type":"Node","parent":""}]})");
		Write(project / "other.tscn", R"({"nodes":[{"name":"Other","type":"Node","parent":""}]})");
		Write(project / "autoload.tscn", R"({"nodes":[{"name":"Session","type":"Node","parent":""}]})");
		if (module == L"--verify-managed")
		{
			Write(project / "main.tscn", R"({"nodes":[{"name":"Main","type":"Node","parent":"","properties":{"script":"res://ManagedLifecycleProbe.cs"}}]})");
		}
		if (module == L"--verify-arguments")
		{
			VerifyArguments(project);
		}
		else if (module == L"--verify-services-only")
		{
			ClassDatabase::Startup();
			RegisterAllTypes();
			EngineConfig config;
			config.m_eventSystemConfig.m_isEnable = false;
			config.m_windowConfig.m_isEnable = false;
			config.m_fileSystemConfig.m_isEnable = false;
			config.m_scriptSystemConfig.m_isEnabled = false;
			config.m_rendererServerConfig.m_isEnable = false;
			config.m_inputConfig.m_isEnable = false;
			config.m_audioConfig.m_isEnable = false;
			config.m_networkConfig.m_isEnable = false;
			{
				Engine engine(config);
				engine.Startup();
				Check(engine.m_sceneSystem->GetSceneTree() == nullptr, "disabled runtime services do not create a window tree");
				engine.BeginFrame();
				engine.Update(1.f / 60.f);
				engine.Render();
				engine.EndFrame();
				engine.Shutdown();
			}
			g_engine = nullptr;
			ClassDatabase::Shutdown();
		}
		else if (module == L"--verify-failed-enter" || module == L"--verify-failed-config")
		{
			Write(project / "project.ming", module == L"--verify-failed-config" ? "{invalid"
				: R"({"type":"ProjectSettings","version":1,"start_scene_path":"res://missing.tscn","autoloads":[{"name":"Session","path":"res://autoload.tscn"}]})");
			MingRunConfig invalid;
			invalid.mode = MingRunMode::Game;
			invalid.projectPath = project;
			App failed(invalid);
			failed.Startup();
			Check(failed.IsQuitting() && failed.GetExitCode() != 0, "failed enter reports failure");
			failed.RunFrame();
			failed.Shutdown();
			failed.Shutdown();
			Check(failed.GetEngine()->m_sceneSystem->GetSceneTree() == nullptr, "failed startup cleans tree and autoloads");
			Check(!failed.GetEngine()->m_physicsServer->Step(1.f / 60.f), "failed startup closes physics world");
		}
		else if (module == L"--verify-physics" || module == L"--verify-managed")
		{
			MingRunConfig config;
			config.mode = MingRunMode::Game;
			config.projectPath = project;
			if (module == L"--verify-physics")
			{
				VerifyPhysics(config);
			}
			else
			{
				VerifyManaged(config, project);
			}
		}
		else
		{
			MingRunConfig config;
			if (module == L"--verify-editor")
			{
				config.mode = MingRunMode::Editor;
			}
			else if (module == L"--verify-game")
			{
				config.mode = MingRunMode::Game;
			}
			else
			{
				Check(module == L"--verify-project-manager", "known verification module");
			}

			if (config.mode != MingRunMode::ProjectManager)
			{
				config.projectPath = project;
			}
			VerifyMode(config);
		}
		std::printf("APPLICATION_LIFECYCLE: PASS checks=%d\n", checks);
		return 0;
	}
	catch (std::exception const& error)
	{
		std::fprintf(stderr, "APPLICATION_LIFECYCLE: FAIL %s\n", error.what());
		return 1;
	}
}
