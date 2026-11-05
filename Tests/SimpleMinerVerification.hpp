#pragma once

#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"

inline int VerifySimpleMinerEditorStop(std::filesystem::path const& project)
{
	MingRunConfig config;
	config.mode = MingRunMode::Editor;
	config.projectPath = std::filesystem::absolute(project);
	App app(config);
	g_app = &app;
	app.Startup();
	bool editorSkippedAutoLoads = app.GetSceneTree()->GetRoot()->FindChildByName("MinerEngine") == nullptr;
	auto* input = g_engine->m_inputSystem;
	input->SetCursorMode(CursorMode::FPS);
	InputEvent focus;
	focus.m_type = InputEventType::WindowFocus;
	focus.m_focused = false;
	input->AddInputEvent(focus);
	app.RunFrame();
	bool editorReleased = glfwGetInputMode(g_engine->m_windowSystem->GetGLFWWindow(), GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
	focus.m_focused = true;
	input->AddInputEvent(focus);
	app.RunFrame();
	EditorNode::Get()->RequestPlay();
	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
	std::string result;
	while (std::chrono::steady_clock::now() < deadline && result != "PASS" && result != "FAIL")
	{
		app.RunFrame();
		std::ifstream(project / "runtime-editor.txt") >> result;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	bool started = app.IsGameRunning() && result == "PASS";
	app.StopGame();
	deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (app.IsGameRunning() && std::chrono::steady_clock::now() < deadline)
	{
		app.RunFrame();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	int x = 0;
	int y = 0;
	int type = 0;
	std::ifstream(project / "runtime-save.txt") >> x >> y >> type;
	bool saved = std::filesystem::exists(project / "Saves" / ("Chunk(" + std::to_string(x) + "," + std::to_string(y) + ").chunk"));
	bool stopped = !app.IsGameRunning();
	app.Shutdown();
	g_app = nullptr;
	std::printf("SIMPLEMINER_EDITOR_STOP started=%d stopped=%d saved=%d focus_released=%d\n", started, stopped, saved, editorReleased);
	return started && stopped && saved && editorReleased && editorSkippedAutoLoads ? 0 : 1;
}

inline int VerifySimpleMinerRuntime(std::filesystem::path const& project)
{
	int failures = 0;
	auto check = [&](bool value, char const* message)
	{
		std::printf("%s: %s\n", value ? "PASS" : "FAIL", message);
		std::fflush(stdout);
		failures += value ? 0 : 1;
	};

	MingRunConfig config;
	config.mode = MingRunMode::Game;
	config.projectPath = std::filesystem::absolute(project);
	App app(config);
	g_app = &app;
	app.Startup();
	if (app.IsQuitting() || app.GetSceneTree() == nullptr)
	{
		app.Shutdown();
		g_app = nullptr;
		return 1;
	}

	auto* input = g_engine->m_inputSystem;
	check(ClassDatabase::GetGlobalObject("InputSystem") == input
		&& ClassDatabase::GetGlobalObject("FileSystem") == g_engine->m_fileSystem,
		"global objects resolve engine-owned systems");
	check(ClassDatabase::GetConstructor("InputSystem") == nullptr
		&& ClassDatabase::GetConstructor("FileSystem") == nullptr,
		"engine-owned systems have no reflected constructor");

	auto* window = g_engine->m_windowSystem->GetGLFWWindow();
	glfwSetKeyCallback(window, nullptr);
	glfwSetCursorPosCallback(window, nullptr);
	glfwSetMouseButtonCallback(window, nullptr);
	glfwSetWindowFocusCallback(window, nullptr);
	InputEvent focus;
	focus.m_type = InputEventType::WindowFocus;
	focus.m_focused = true;
	input->ProcessInputEvent(focus);
	input->SetCursorMode(CursorMode::FPS);
	input->ClearCursorDelta();
	InputEvent motion;
	motion.m_type = InputEventType::MouseMotion;
	motion.m_relative = Vector2(0.25f, -0.125f);
	input->ProcessInputEvent(motion);
	input->ProcessInputEvent(motion);
	auto* getter = ClassDatabase::GetMethodBind("InputSystem", "GetMouseDelta");
	Vector2 delta;
	getter->PtrCall(input, nullptr, &delta);
	check(delta == Vector2(0.5f, -0.25f) && input->GetMouseDelta() == delta,
		"registered mouse getter preserves fractions and does not consume motion");
	InputEvent key;
	key.m_type = InputEventType::Key;
	key.m_keyCode = 'W';
	key.m_pressed = true;
	input->ProcessInputEvent(key);
	focus.m_focused = false;
	input->ProcessInputEvent(focus);
	check(!input->IsKeyDown('W') && input->GetMouseDelta() == Vector2::Zero, "focus loss clears held keys and motion");
	focus.m_focused = true;
	input->ProcessInputEvent(focus);
	input->ProcessInputEvent(motion);
	input->BeginFrame();
	check(input->GetMouseDelta() == Vector2::Zero, "new input frame clears previous mouse motion");

	Vector3 initialPosition;
	float initialYaw = 0.f;
	double totalFrameMs = 0.0;
	double maxFrameMs = 0.0;
	for (int frame = 0; frame < 128; ++frame)
	{
		if (frame == 0 || frame == 1)
		{
			focus.m_focused = frame == 1;
			input->AddInputEvent(focus);
		}

		if (frame == 1 || frame == 3)
		{
			key.m_pressed = frame == 1;
			input->AddInputEvent(key);
		}

		if (frame == 4)
		{
			input->AddInputEvent(motion);
		}

		auto start = std::chrono::steady_clock::now();
		app.RunFrame();
		double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
		if (frame == 0)
		{
			check(!input->IsWindowFocused() && glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED,
				"game focus loss preserves the requested FPS mode for focus recovery");
		}

		totalFrameMs += ms;
		maxFrameMs = (std::max)(maxFrameMs, ms);
		auto* camera = app.GetSceneTree()->GetWorldCamera();
		if (frame == 0 && camera != nullptr)
		{
			initialPosition = camera->GetLocalPosition();
			initialYaw = camera->GetLocalOrientation().m_yawDegrees;
		}

		if (frame == 5)
		{
			check(camera != nullptr && camera->GetLocalPosition() != initialPosition
				&& camera->GetLocalOrientation().m_yawDegrees != initialYaw,
				"scripted key and fractional mouse events drive the C# camera");
		}
	}

	std::printf("SIMPLEMINER_FRAMES count=128 mean_ms=%.3f max_ms=%.3f\n", totalFrameMs / 128.0, maxFrameMs);

	auto* scene = app.GetSceneTree()->GetScene();
	std::vector<ObjectID> meshIDs;
	bool geometry = false;
	if (scene != nullptr)
	{
		Node* voxelRoot = scene->GetNode(NodePath("VoxelRoot"));
		check(voxelRoot != nullptr, "Game exposes its voxel presentation root");
		for (Node* child : voxelRoot != nullptr ? voxelRoot->GetChildren() : std::vector<Node*>())
		{
			auto* instance = dynamic_cast<MeshInstance3D*>(child);
			if (instance == nullptr || !instance->GetMeshResource().IsValid())
			{
				continue;
			}

			auto mesh = instance->GetMeshResource();
			meshIDs.push_back(mesh->GetObjectID());
			auto const* vertices = reinterpret_cast<Vertex const*>(mesh->m_vertices.data());
			geometry = mesh->m_vertexCount > 0 && mesh->m_vertexCount % 4 == 0
				&& mesh->m_indexCount == mesh->m_vertexCount / 4 * 6 && mesh->GetMaterial().IsValid()
				&& mesh->GetMaterial()->GetDiffuseTexture().IsValid()
				&& mesh->GetMaterial()->GetSamplerMode() == SamplerMode::POINT_CLAMP
				&& (vertices[0].m_color == Color(250, 250, 250) || vertices[0].m_color == Color(230, 230, 230)
					|| vertices[0].m_color == Color(200, 200, 200));
			if (!geometry)
			{
				break;
			}
		}
	}

	check(geometry && !meshIDs.empty(), "C# terrain creates textured colored indexed chunk meshes");
	auto* viewport = static_cast<Viewport*>(app.GetSceneTree()->GetRoot());
	std::filesystem::create_directories(project / ".ming/Verification");
	SaveRenderedFrame(g_engine->m_renderServer->ViewportGetTexture(viewport->GetViewportRID()),
		project / ".ming/Verification/terrain.ppm");
	app.Shutdown();
	check(!input->IsNativeBindingGCHandleValid() && !g_engine->m_fileSystem->IsNativeBindingGCHandleValid(),
		"shutdown invalidates global managed wrappers before stopping the bridge");
	for (ObjectID id : meshIDs)
	{
		check(ObjectDatabase::GetInstance(id) == nullptr, "shutdown releases chunk mesh registration");
	}

	check(ClassDatabase::GetGlobalObject("InputSystem") == nullptr, "shutdown removes global object registrations");
	char phase[32]{};
	GetEnvironmentVariableA("MING_SIMPLEMINER_PHASE", phase, sizeof(phase));
	std::string result;
	std::ifstream(project / (std::string("runtime-") + phase + ".txt")) >> result;
	check(result == "PASS", "managed input, frame budgets, editing and persistence checks");
	std::string exited;
	std::ifstream(project / (std::string("runtime-exit-") + phase + ".txt")) >> exited;
	check(exited == "PASS", "current world ends before autoload tree shutdown and static reference clears");
	g_app = nullptr;
	std::printf("SIMPLEMINER_RUNTIME phase=%s failures=%d\n", phase, failures);
	return failures == 0 ? 0 : 1;
}
