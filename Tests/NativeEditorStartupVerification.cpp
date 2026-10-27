#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "RenderReadback.hpp"

#include <cstdio>

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		return 2;
	}

	std::string const mode = argv[2];
	MingRunConfig     config;
	config.mode = mode == "manager"  ? MingRunMode::ProjectManager
				  : mode == "editor" ? MingRunMode::Editor
									 : MingRunMode::Game;
	if (config.mode != MingRunMode::ProjectManager)
	{
		config.projectPath = std::filesystem::absolute(argv[1]) / "Startup";
		std::filesystem::create_directories(config.projectPath);
		for (char const* file : { "demo.mesh", "native-demo.tscn" })
		{
			std::filesystem::copy_file(
				std::filesystem::absolute(argv[1]) / file,
				config.projectPath / file,
				std::filesystem::copy_options::overwrite_existing);
		}

		std::ofstream settings(config.projectPath / "project.ming");
		settings << R"({"type":"ProjectSettings","version":1,"start_scene_path":"res://native-demo.tscn"})";
	}

	App app(config);
	app.Startup();
	for (int i = 0; i < 3; ++i)
	{
		app.RunFrame();
	}

	bool const                  native   = g_engine->m_imguiSystem == nullptr && ImGui::GetCurrentContext() == nullptr;
	bool const                  scene    = app.GetSceneTree() != nullptr;
	std::filesystem::path const captures = std::filesystem::absolute(argv[1]).parent_path() / "Captures";
	std::filesystem::create_directories(captures);
	if (scene)
	{
		auto* root = static_cast<Viewport*>(app.GetSceneTree()->GetRoot());
		SaveRenderedFrame(
			g_engine->m_renderServer->ViewportGetTexture(root->GetViewportRID()),
			captures / ("startup-" + mode + ".ppm"));
	}

	bool const editor = config.mode != MingRunMode::Editor || EditorNode::Get() != nullptr;
	app.Quit();
	bool const shutdown = app.IsQuitting();
	app.Shutdown();
	std::printf(
		"NATIVE_STARTUP mode=%s no_imgui=%d scene_tree=%d editor=%d quit=%d\n",
		mode.c_str(),
		native,
		scene,
		editor,
		shutdown);
	return native && scene && editor && shutdown ? 0 : 1;
}
