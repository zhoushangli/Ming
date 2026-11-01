#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
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
		std::ofstream scene(config.projectPath / "native-demo.tscn");
		scene << R"({"nodes":[{"name":"StartupScene","type":"Node3D","parent":""}]})";
		scene.close();

		std::ofstream settings(config.projectPath / "project.ming");
		settings << R"({"type":"ProjectSettings","version":1,"start_scene_path":"res://native-demo.tscn"})";
	}

	App app(config);
	app.Startup();
	for (int i = 0; i < 3; ++i)
	{
		app.RunFrame();
	}

	bool const                  scene    = app.GetSceneTree() != nullptr;
	std::filesystem::path const captures = std::filesystem::absolute(argv[1]).parent_path() / "Captures";
	std::filesystem::create_directories(captures);
	if (scene)
	{
		auto* root = static_cast<Viewport*>(app.GetSceneTree()->GetRoot());
		SaveRenderedFrame(
			g_engine->m_renderServer->ViewportGetTexture(root->GetViewportRID()),
			captures / ("startup-" + mode + ".ppm"));
		if (config.mode == MingRunMode::Editor)
		{
			for (IntVec2 size : { IntVec2(1440, 900), IntVec2(960, 600) })
			{
				root->UpdateResolution(size);
				EditorNode::Get()->m_editorUI->ResetNativeLayout();
				EditorNode::Get()->GetSelection().SetSelected(app.GetSceneTree()->GetScene()->GetObjectID());
				for (int frame = 0; frame < 3; ++frame)
				{
					app.RunFrame();
				}

				SaveRenderedFrame(g_engine->m_renderServer->ViewportGetTexture(root->GetViewportRID()),
					captures / ("warm-carbon-" + std::to_string(size.x) + "x" + std::to_string(size.y) + ".ppm"));
			}
		}
	}

	bool const editor = config.mode != MingRunMode::Editor || EditorNode::Get() != nullptr;
	app.Quit();
	bool const shutdown = app.IsQuitting();
	app.Shutdown();
	std::printf(
		"NATIVE_STARTUP mode=%s scene_tree=%d editor=%d quit=%d\n",
		mode.c_str(),
		scene,
		editor,
		shutdown);
	return scene && editor && shutdown ? 0 : 1;
}
