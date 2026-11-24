#define NOMINMAX
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Scene/SceneSystem.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Core/IO/ResourceImporter.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/IO/Image.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"
#include "MingEngine/Core/OS/WorkerThreadPool.hpp"
#include "ThirdParty/GLFW/glfw3.h"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <thread>
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

void Write(std::filesystem::path const& path, std::string const& text)
{
	std::ofstream output(path);
	output << text;
	Check(output.good(), "write fixture");
}

void Image(std::filesystem::path const& path, uint8_t red)
{
	uint8_t bytes[21] = {};
	bytes[2] = 2;
	bytes[12] = 1;
	bytes[14] = 1;
	bytes[16] = 24;
	bytes[20] = red;
	std::ofstream output(path, std::ios::binary);
	output.write(reinterpret_cast<char const*>(bytes), sizeof(bytes));
}

void Wait(App& app, std::function<bool()> const& condition, char const* message)
{
	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
	while (!condition() && std::chrono::steady_clock::now() < deadline)
	{
		app.RunFrame();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	Check(condition(), message);
}

Button* FindButton(Node* node, String const& text)
{
	if (node == nullptr)
	{
		return nullptr;
	}
	if (auto* button = dynamic_cast<Button*>(node); button != nullptr && button->GetText() == text)
	{
		return button;
	}
	for (Node* child : node->GetChildren())
	{
		if (auto* found = FindButton(child, text))
		{
			return found;
		}
	}
	return nullptr;
}

void Choose(Viewport* viewport, String const& text)
{
	auto* button = FindButton(viewport->GetGuiPopup(), text);
	Check(button != nullptr, "dialog choice exists");
	button->GrabFocus();
	InputEvent event;
	event.m_type = InputEventType::MouseButton;
	event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
	event.m_position = button->GetSize() * 0.5f;
	event.m_pressed = true;
	button->GuiInput(event);
	event.m_pressed = false;
	button->GuiInput(event);
}

uint64_t Key(Node* node)
{
	auto const id = node->GetObjectID();
	return ((static_cast<uint64_t>(id.GetUID()) << 32) | id.GetIndex()) + 1;
}

void Verify()
{
	auto const directory = std::filesystem::absolute("../Temporary/EditorProjectVerification/Project" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::create_directories(directory / "Settings");
	_wputenv_s(L"LOCALAPPDATA", (directory / "Settings").c_str());
	std::string const scene = R"({"nodes":[{"name":"Main","type":"Node","parent":""},{"name":"A","type":"Node","parent":"."},{"name":"B","type":"Node","parent":"."},{"name":"C","type":"Node","parent":"."}]})";
	Write(directory / "project.ming", R"({"type":"ProjectSettings","version":1,"start_scene_path":"res://main.tscn"})");
	Write(directory / "main.tscn", scene);
	Image(directory / "board.tga", 12);
	MingRunConfig config;
	config.mode = MingRunMode::Editor;
	config.projectPath = directory;
	App app(config);
	app.Startup();
	Check(!app.IsQuitting(), "editor startup");
	glfwHideWindow(app.GetEngine()->m_windowSystem->GetGLFWWindow());
	auto* editor = EditorNode::Get();
	auto* viewport = static_cast<Viewport*>(app.GetEngine()->m_sceneSystem->GetSceneTree()->GetRoot());
	Wait(app, [editor]() { return editor->HasScene() && !editor->IsCheckingProject(); }, "initial imports finish before scene opens");
	Check(std::filesystem::exists(directory / "board.tga.import"), "initial source imported");
	auto* selected = &editor->GetSelection();
	Node* root = editor->GetSceneTree()->GetScene();
	Node* c = root->FindChildByName("C");
	Node* b = root->FindChildByName("B");
	app.RunFrame();
	Tree* tree = editor->m_editorUI->GetSceneTreeControl();
	GuiDragData drag{ "node", c->GetObjectID(), "C" };
	auto rect = tree->GetItemRect(Key(b));
	Vector2 above(rect.GetCenter().x, rect.m_mins.y + 1.f);
	Check(tree->GetDropSectionAtPosition(above) == -1, "tree detects insertion above");
	Check(tree->CanDropData(above, drag), "sibling reorder accepted");
	tree->DropData(above, drag);
	Check(root->GetChildren()[1] == c && c->GetParent() == root, "sibling reorder preserves hierarchy");
	editor->GetUndoRedo().Undo(root->GetObjectID());
	Check(root->GetChildren()[2] == c, "reorder undo restores position");
	editor->GetUndoRedo().Redo(root->GetObjectID());
	Check(root->GetChildren()[1] == c, "reorder redo restores position");
	Check(editor->SaveScene(), "save edited scene");
	editor->CheckProject();
	for (int i = 0; i < 40; ++i)
	{
		app.RunFrame();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	Check(viewport->GetGuiPopup() == nullptr, "own save does not show external conflict");

	Ref<TextureResource> texture(ResourceLoader::Load("res://board.tga"));
	Check(texture.IsValid(), "load imported texture");
	auto* textureIdentity = texture.Get();
	auto oldTime = std::filesystem::last_write_time(directory / "board.tga");
	Image(directory / "board.tga", 220);
	std::filesystem::last_write_time(directory / "board.tga", oldTime - std::chrono::seconds(20));
	editor->CheckProject();
	Wait(app, [&texture]() { return texture->m_image->GetRawData()[0] == 220; }, "time rollback triggers texture reimport and reload");
	Check(texture.Get() == textureIdentity, "reload retains resource identity");
	VirtualPath outputPath;
	Check(ResourceImporter::TryGetImportFile("res://board.tga", outputPath), "resolve generated texture");
	std::filesystem::path outputPhysical;
	app.GetEngine()->m_fileSystem->TryGetPhysicalPath(outputPath, outputPhysical);
	std::filesystem::remove(outputPhysical);
	editor->CheckProject();
	Wait(app, [&outputPhysical]() { return std::filesystem::exists(outputPhysical); }, "missing imported product is regenerated");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "recovery completes");

	Write(directory / "main.tscn", R"({"nodes":[{"name":"External","type":"Node","parent":""}]})");
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "external scene change prompts");
	Choose(viewport, "Reload from disk");
	Wait(app, [editor]() { return editor->GetSceneTree()->GetScene()->GetName() == "External"; }, "reload chooses disk scene");
	Check(&editor->GetSelection() == selected, "selection survives scene reload");
	Write(directory / "main.tscn", scene);
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "second external scene change prompts");
	Choose(viewport, "Ignore external changes");
	std::ifstream saved(directory / "main.tscn");
	std::string savedText((std::istreambuf_iterator<char>(saved)), {});
	Check(savedText.find("External") != std::string::npos, "ignore overwrites disk with editor scene");
	Node* retained = editor->GetSceneTree()->GetScene();
	Write(directory / "main.tscn", "{");
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "invalid external scene still prompts");
	Choose(viewport, "Reload from disk");
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "OK") != nullptr; }, "reload failure is reported");
	Check(editor->GetSceneTree()->GetScene() == retained, "failed reload retains edited scene");
	Choose(viewport, "OK");
	Check(editor->SaveScene(), "save retained scene after failed reload");


	Ref<ProjectSettings> settings = ProjectSettings::Get();
	Write(directory / "project.ming", R"({"type":"ProjectSettings","version":1,"start_scene_path":""})");
	editor->CheckProject();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Reload from disk") != nullptr; }, "project settings change prompts");
	Choose(viewport, "Reload from disk");
	Check(ProjectSettings::Get().Get() == settings.Get() && !settings->m_startScenePath.IsValid(), "configuration reload retains identity");

	std::filesystem::create_directories(directory.parent_path() / "External");
	auto const external = directory.parent_path() / "External" / "drop.tga";
	Image(external, 44);
	Tree* files = editor->m_editorUI->GetFileTreeControl();
	Vector2 const drop = files->GetGlobalPosition() + Vector2(files->GetSize().x * 0.5f, files->GetSize().y - 8.f);
	auto importFile = [&]()
	{
		std::string utf8 = external.u8string();
		// 1) Exercise the same editor callback used by GLFW, with an explicit drop position.
		editor->ImportExternalFiles({ utf8 }, drop);
	};
	importFile();
	Wait(app, [&directory]() { return std::filesystem::exists(directory / "drop.tga.import"); }, "external file copied and imported");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "external import completes");
	Image(external, 88);
	importFile();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Skip") != nullptr; }, "copy collision prompts");
	Choose(viewport, "Skip");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "skipped copy completes");
	Ref<TextureResource> dropped(ResourceLoader::Load("res://drop.tga"));
	Check(dropped->m_image->GetRawData()[0] == 44, "skip preserves existing file");
	importFile();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Skip") != nullptr; }, "second collision prompts");
	Choose(viewport, "Replace");
	Wait(app, [&dropped]() { return dropped->m_image->GetRawData()[0] == 88; }, "replace copies and updates loaded resource");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "replacement finishes");

	Write(directory / "Probe.cs", "using Ming; public partial class Probe : Node { }\n");
	Image(external, 120);
	importFile();
	Wait(app, [viewport]() { return FindButton(viewport->GetGuiPopup(), "Cancel") != nullptr; }, "cancel copy collision prompts");
	Choose(viewport, "Cancel");
	Check(dropped->m_image->GetRawData()[0] == 88, "cancel preserves destination");
	auto const folder = external.parent_path() / "Folder";
	std::filesystem::create_directories(folder);
	auto const unicodeName = std::filesystem::u8path(u8"\u4e2d\u6587.tga");
	Image(folder / unicodeName, 33);
	editor->ImportExternalFiles({ folder.u8string() }, drop);
	Wait(app, [&directory, &unicodeName]() { return std::filesystem::exists(directory / "Folder" / (unicodeName.wstring() + L".import")); }, "recursive copy imports Unicode filename");
	Wait(app, [editor]() { return !editor->IsCheckingProject(); }, "directory import completes");
	Check(editor->GenerateCSharpProject(), "generate C# project");
	Check(editor->BuildCSharpProject(), "start editor build");
	Wait(app, [editor]() { return !editor->IsBuildingCSharpProject(); }, "editor build completes");
	Check(editor->GetCSharpBuildExitCode() == 0 && viewport->GetGuiPopup() == nullptr, "C# build succeeds with editor pool");
	Check(editor->ReloadCSharpAssembly(), "native editor pool allows assembly reload");
	Check(editor->SetMainScene("res://main.tscn"), "set main scene for Play");
	editor->RequestPlay();
	Wait(app, [&app]() { return app.IsGameRunning(); }, "Play saves builds validates and starts game");
	editor->RequestStop();
	Wait(app, [&app]() { return !app.IsGameRunning(); }, "Stop reaps the game process");
	auto* pool = app.GetEngine()->m_workerThreadPool;
	std::atomic<int> result{0};
	auto id = pool->AddNativeTask([](void* data) { static_cast<std::atomic<int>*>(data)->store(1); }, &result);
	Check(id != WorkerThreadPool::INVALID_TASK_ID && !pool->HasManagedTasks(), "native pool task accepted without managed callbacks");
	Check(pool->WaitForTaskCompletion(id) && result == 1, "native task completes and is reclaimed");
	Check(!pool->WaitForTaskCompletion(id), "task cannot be reclaimed twice");
	editor->CheckProject();
	app.RunFrame();
	dropped = {};
	texture = {};
	settings = {};
	app.Shutdown();
	Check(EditorNode::Get() == nullptr, "shutdown drains checker before destroying editor");
}
}

int wmain(int argc, wchar_t** argv)
{
	if (argc > 1)
	{
		MingRunConfig config;
		if (!TryParseRunConfig(config, argc, argv))
		{
			return 2;
		}
		App child(config);
		child.Startup();
		glfwHideWindow(child.GetEngine()->m_windowSystem->GetGLFWWindow());
		child.RunMainLoop();
		child.Shutdown();
		return child.GetExitCode();
	}

	try
	{
		Verify();
		std::printf("EDITOR_PROJECT: PASS checks=%d\n", checks);
		return 0;
	}
	catch (std::exception const& exception)
	{
		std::fprintf(stderr, "EDITOR_PROJECT: FAIL %s\n", exception.what());
		return 1;
	}
}
