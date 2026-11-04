#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/Script/CSharpScriptGenerator.hpp"
#include "ThirdParty/tinyxml2/tinyxml2.h"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/CreateScriptPopup.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/MeshInstance3D.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "ThirdParty/GLFW/glfw3.h"
#include <fstream>
#include <cstdio>
#include <chrono>
#include <thread>
#include <cstring>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#undef GetClassName
#include "RenderReadback.hpp"

template <typename T, typename Predicate>
T* FindScriptControl(Node* root, Predicate predicate)
{
	if (root == nullptr)
	{
		return nullptr;
	}

	if (auto* value = dynamic_cast<T*>(root); value != nullptr && !value->IsQueuedForDeletion() && predicate(value))
	{
		return value;
	}

	for (Node* child : root->GetChildren())
	{
		if (auto* found = FindScriptControl<T>(child, predicate))
		{
			return found;
		}
	}

	return nullptr;
}

int main(int argc, char** argv)
{
	if (argc == 3)
	{
		int count = 0;
		wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &count);
		if (count == 3 && std::filesystem::path(arguments[2]).extension() == L".cs")
		{
			std::ofstream(std::filesystem::path(arguments[1]) / "EditorLaunch.txt")
				<< (GetEnvironmentVariableW(L"ELECTRON_RUN_AS_NODE", nullptr, 0) == 0
					? std::filesystem::path(arguments[2]).generic_u8string() : "Unexpected Electron node mode");
			LocalFree(arguments);
			return 0;
		}

		LocalFree(arguments);
	}

	if (argc == 4 && std::string(argv[1]) == "--game" && std::string(argv[2]) == "--project")
	{
		MingRunConfig child;
		child.mode = MingRunMode::Game;
		int count = 0;
		wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &count);
		child.projectPath = arguments[3];
		LocalFree(arguments);
		return MingEngine::Run(child);
	}

	if (argc != 2)
	{
		return 2;
	}

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
	auto const project = std::filesystem::absolute(argv[1]) / std::filesystem::u8path("CSharp Workflow 空项目");
	std::filesystem::create_directories(project / "Scripts");
	std::ofstream(project / "project.ming") << R"({"type":"ProjectSettings","version":1,"start_scene_path":""})";
	std::filesystem::create_directories(project / "UserData");
	_wputenv_s(L"LOCALAPPDATA", (project / "UserData").c_str());
	_wputenv_s(L"MING_WORKFLOW_PROJECT", project.c_str());
	MingRunConfig config;
	config.mode = MingRunMode::Editor;
	config.projectPath = project;
	App app(config);
	g_app = &app;
	app.Startup();
	auto* editor = EditorNode::Get();
	auto* scripts = g_engine->m_scriptSystem;
	auto* viewport = static_cast<Viewport*>(app.GetSceneTree()->GetRoot());
	auto* window = g_engine->m_windowSystem->GetGLFWWindow();
	glfwSetWindowFocusCallback(window, nullptr);
	glfwSetFramebufferSizeCallback(window, nullptr);
	{
		auto const directory = project.parent_path() / "Documentation";
		auto const docs = directory / "Docs/Classes";
		std::filesystem::create_directories(docs);
		std::error_code ignored;
		std::filesystem::remove(docs / "Node.xml", ignored);
		CSharpScriptGenerator generator;
		check(generator.GenerateCSharpBindings(directory), "API without docs still generates");
		auto readText = [](std::filesystem::path const& path)
		{
			std::ifstream input(path, std::ios::binary);
			return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
		};
		auto const original = readText(directory / "Generated/Node.cs");
		std::ofstream(docs / "Node.xml") << "<class name=\"Node\"><methods>";
		check(!generator.GenerateCSharpBindings(directory)
			&& readText(directory / "Generated/Node.cs") == original,
			"bad XML does not remove old bindings");
		std::ofstream(docs / "Node.xml") << R"(<class name="Node"><methods><method name="AddNode">
<param name="wrongName">The child.</param></method></methods></class>)";
		check(!generator.GenerateCSharpBindings(directory)
			&& readText(directory / "Generated/Node.cs") == original,
			"wrong parameter name does not remove old bindings");
		std::ofstream(docs / "Node.xml") << R"(<class name="Node"><brief_description>A &lt;node&gt; &amp; its children.</brief_description>
<methods><method name="AddNode"><description>Adds a &lt;child&gt; &amp; keeps its name.</description>
<param name="child">The node to add.</param></method></methods></class>)";
		check(generator.GenerateCSharpBindings(directory), "valid class and method docs generate");
		auto const generated = readText(directory / "Generated/Node.cs");
		check(generated.find("/// <summary>A &lt;node&gt; &amp; its children.</summary>") != std::string::npos
			&& generated.find("/// <param name=\"child\">The node to add.</param>") != std::string::npos
			&& generated.find("public void AddNode(Node child)") != std::string::npos,
			"docs escape XML and preserve the public signature");
		check(readText(directory / "Generated/Node3D.cs").find("public Vector3 GetLocalPosition()") != std::string::npos,
			"a class without docs keeps its methods");

		tinyxml2::XMLDocument deployed;
		auto const xml = readText(std::filesystem::current_path() / "MingSharp/Api/MingSharp.xml");
		bool loaded = deployed.Parse(xml.c_str(), xml.size()) == tinyxml2::XML_SUCCESS;
		bool hasNode = false;
		bool hasNode3D = false;
		bool hasCamera = false;
		bool hasInput = false;
		bool hasResource = false;
		bool hasButton = false;
		auto const* members = loaded && deployed.RootElement() != nullptr
			? deployed.RootElement()->FirstChildElement("members") : nullptr;
		for (auto const* member = members != nullptr ? members->FirstChildElement("member") : nullptr;
			member != nullptr; member = member->NextSiblingElement("member"))
		{
			std::string const name = member->Attribute("name") != nullptr ? member->Attribute("name") : "";
			hasNode = hasNode || (name == "M:Ming.Node.AddNode(Ming.Node)" && member->FirstChildElement("param") != nullptr);
			hasNode3D = hasNode3D || (name == "M:Ming.Node3D.GetLocalPosition" && member->FirstChildElement("returns") != nullptr);
			hasCamera = hasCamera || (name == "M:Ming.Camera3D.SetFovDegrees(System.Single)"
				&& member->FirstChildElement("param") != nullptr);
			hasInput = hasInput || (name == "M:Ming.InputSystem.WasKeyJustPressed(System.Int32)"
				&& member->FirstChildElement("returns") != nullptr);
			hasResource = hasResource || (name == "M:Ming.Resource.SetPath(System.String)"
				&& member->FirstChildElement("param") != nullptr);
			hasButton = hasButton || (name == "T:Ming.Button" && member->FirstChildElement("summary") != nullptr);
		}

		check(loaded && hasNode && hasNode3D, "deployed MingSharp.xml contains core API docs");
		check(loaded && hasCamera && hasInput && hasResource && hasButton,
			"deployed docs cover camera, input, resources, and a class with inherited methods");
	}

	CreateScriptPopup creator;
	std::string error;
	std::error_code ignored;
	std::filesystem::remove(project / "Scripts/Mover.cs", ignored);
	std::filesystem::remove(project / "Scripts/Replacement.cs", ignored);
	check(creator.Create("res://Scripts/Mover.cs", "Node3D", true, error), "create C# script and net9 project");
	check(!creator.Create("res://Scripts/Mover.cs", "Node3D", true, error), "duplicate script does not overwrite");
	check(!creator.Create("res://Scripts/class.cs", "Node3D", true, error), "keyword cannot become a script class");
	check(!creator.Create("res://Scripts/Wrong.cs", "NoSuchType", true, error), "unbound base type is rejected");
	{
		auto const bin = project / "Mock VS Code/bin";
		std::filesystem::create_directories(bin);
		wchar_t executable[32768]{};
		GetModuleFileNameW(nullptr, executable, 32768);
		std::filesystem::copy_file(executable, bin / "Code.exe", std::filesystem::copy_options::overwrite_existing);
		std::wstring const oldPath = _wgetenv(L"PATH") != nullptr ? _wgetenv(L"PATH") : L"";
		std::wstring const oldNodeMode = _wgetenv(L"ELECTRON_RUN_AS_NODE") != nullptr ? _wgetenv(L"ELECTRON_RUN_AS_NODE") : L"";
		_wputenv_s(L"PATH", (bin.wstring() + L";" + oldPath).c_str());
		_wputenv_s(L"ELECTRON_RUN_AS_NODE", L"1");
		auto const record = project / "EditorLaunch.txt";
		std::filesystem::remove(record, ignored);
		bool const started = editor->OpenCSharpScript("res://Scripts/Mover.cs");
		_wputenv_s(L"PATH", oldPath.c_str());
		_wputenv_s(L"ELECTRON_RUN_AS_NODE", oldNodeMode.c_str());
		std::string recorded;
		auto const expected = (project / "Scripts/Mover.cs").generic_u8string();
		for (int attempt = 0; attempt < 200 && recorded != expected; ++attempt)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			std::ifstream input(record);
			recorded.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
		}

		check(started && recorded == expected, "editor receives game folder and Unicode script without Electron node mode");
	}

	check(scripts->BuildProjectSolution(), "new script template compiles without manual path attributes");
	auto writeScript = [&](int speed, std::string const& extra = "")
	{
		std::ofstream output(project / "Scripts/Mover.cs");
		output << "using Ming;\nnamespace Workflow.Sample;\npublic partial class Mover : Node3D\n{\n"
			<< "float elapsed;\npublic override void OnReady() { NativeFuncs.Log(\"CS_READY_" << speed << "\"); }\n"
			<< "public override void OnProcess(float deltaSeconds) { elapsed += deltaSeconds; "
			<< "SetLocalPosition(new Vector3(6, System.MathF.Sin(elapsed) * " << speed << ", 0)); "
			<< "System.IO.File.WriteAllText(System.IO.Path.Combine(System.Environment.GetEnvironmentVariable(\"MING_WORKFLOW_PROJECT\")!, \"progress.txt\"), \""
			<< speed << " \" + GetLocalPosition().Y); " << extra << " }\n}\n";
	};
	writeScript(1);

	auto* root = MemNew<Node3D>();
	root->SetName("CSharpWorkflow");
	auto* camera = MemNew<Camera3D>();
	camera->SetName("Camera");
	root->AddNode(camera);
	auto* moving = MemNew<MeshInstance3D>();
	moving->SetName("MovingCube");
	moving->SetLocalPosition(Vector3(6.f, 0.f, 0.f));
	Ref<MeshResource> mesh = CreateRef<MeshResource>();
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	AddVertsForAABB3D(vertices, indices, AABB3(Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f)), Color(211, 173, 114));
	mesh->m_bounds = AABB3(Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f));
	mesh->m_vertexFormat = "PCUTBN";
	mesh->m_vertexStride = sizeof(Vertex);
	mesh->m_vertexCount = static_cast<uint32_t>(vertices.size());
	mesh->m_vertices.resize(vertices.size() * sizeof(Vertex));
	std::memcpy(mesh->m_vertices.data(), vertices.data(), mesh->m_vertices.size());
	mesh->m_indexCount = static_cast<uint32_t>(indices.size());
	mesh->m_indices.resize(indices.size() * sizeof(unsigned int));
	std::memcpy(mesh->m_indices.data(), indices.data(), mesh->m_indices.size());
	mesh->InitGPUResources();
	check(ResourceSaver::Save("res://cube.mesh", mesh), "fixture prepares its own visible cube");
	mesh->SetVirtualPath("res://cube.mesh");
	moving->SetMeshResource(Variant(mesh));
	root->AddNode(moving);
	app.GetSceneTree()->ChangeScene(root);
	editor->NewRootCreated();
	editor->GetSelection().SetSelected(moving->GetObjectID());
	app.RunFrame();
	app.RunFrame();
	auto* field = FindScriptControl<Button>(editor->m_editorUI->GetRootControl(), [](Button* b) { return b->GetName() == String("ScriptResource"); });
	GuiDragData const data{ "file", ObjectID::Invalid, "res://Scripts/Mover.cs" };
	check(field != nullptr && field->CanDropData(Vector2::Zero, data), "Inspector exposes Script resource drop target before compilation");
	if (field != nullptr)
	{
		field->DropData(Vector2::Zero, data);
	}

	check(moving->GetScriptInstance() != nullptr && editor->IsSceneDirty(), "first drop attaches placeholder and marks scene dirty");
	check(editor->GetUndoRedo().Undo(root->GetObjectID()) && moving->GetScriptInstance() == nullptr, "undo first script assignment");
	check(editor->GetUndoRedo().Redo(root->GetObjectID()) && moving->GetScriptInstance() != nullptr, "redo first script assignment");
	auto press = [&](Button* button)
	{
		check(button != nullptr, "script action is available");
		if (button == nullptr)
		{
			return;
		}

		button->GrabFocus();
		InputEvent key;
		key.m_type = InputEventType::Key;
		key.m_keyCode = GLFW_KEY_ENTER;
		key.m_pressed = true;
		button->GuiInput(key);
		key.m_pressed = false;
		button->GuiInput(key);
		app.RunFrame();
	};
	app.RunFrame();
	press(FindScriptControl<Button>(editor->m_editorUI->GetRootControl(), [](Button* b) { return b->GetName() == String("ScriptMenu"); }));
	press(FindScriptControl<Button>(viewport->GetGuiPopup(), [](Button* b) { return b->GetText() == String("New Script..."); }));
	auto* scriptPath = FindScriptControl<LineEdit>(viewport->GetGuiPopup(), [](LineEdit*) { return true; });
	if (scriptPath != nullptr)
	{
		scriptPath->SetText("res://Scripts/Replacement.cs");
	}

	press(FindScriptControl<Button>(viewport->GetGuiPopup(), [](Button* b) { return b->GetText() == String("Create"); }));
	Ref<Resource> replacement(moving->GetScript());
	check(std::filesystem::exists(project / "Scripts/Replacement.cs") && replacement.IsValid()
		&& replacement->GetVirtualPath() == VirtualPath("res://Scripts/Replacement.cs"),
		"Inspector New Script creates and attaches to its original owner");
	check(editor->GetUndoRedo().Undo(root->GetObjectID())
		&& Ref<Resource>(moving->GetScript())->GetVirtualPath() == VirtualPath("res://Scripts/Mover.cs"),
		"undo script replacement restores previous resource");
	app.RunFrame();
	press(FindScriptControl<Button>(editor->m_editorUI->GetRootControl(), [](Button* b) { return b->GetName() == String("ScriptMenu"); }));
	press(FindScriptControl<Button>(viewport->GetGuiPopup(), [](Button* b) { return b->GetText() == String("Clear"); }));
	check(moving->GetScriptInstance() == nullptr, "Script menu Clear detaches script");
	check(editor->GetUndoRedo().Undo(root->GetObjectID()) && moving->GetScriptInstance() != nullptr,
		"undo Script menu Clear restores attachment");
	check(editor->SaveSceneAs("res://main.tscn") && editor->SetMainScene("res://main.tscn"), "save scripted main scene");
	check(editor->LoadScene("res://main.tscn"), "reopen scripted scene");
	app.RunFrame();
	root = static_cast<Node3D*>(app.GetSceneTree()->GetScene());
	moving = dynamic_cast<MeshInstance3D*>(root->FindChildByName("MovingCube"));
	check(moving != nullptr && moving->GetScriptInstance() != nullptr, "script reference survives scene round trip");

	std::filesystem::create_directories(project / "Captures");
	auto capture = [&](std::string const& name)
	{
		app.RunFrame();
		SaveRenderedFrame(g_engine->m_renderServer->ViewportGetTexture(viewport->GetViewportRID()), project / "Captures" / (name + ".ppm"));
	};
	viewport->UpdateResolution(IntVec2(1440, 900));
	editor->GetSelection().SetSelected(moving->GetObjectID());
	editor->m_editorUI->GetPanel<FileSystemPanel>().NavigateTo("res://Scripts");
	editor->m_editorUI->GetPanel<FileSystemPanel>().SelectPath("res://Scripts/Mover.cs");
	moving->SetScript(Variant(static_cast<Object*>(nullptr)));
	editor->SaveScene();
	app.RunFrame();
	app.RunFrame();
	Tree* files = editor->m_editorUI->GetFileTreeControl();
	files->EnsureSelectedVisible();
	app.RunFrame();
	Vector2 const sourcePosition = files->GetGlobalPosition() + Vector2(30.f, files->GetItemRect(files->GetSelected()).GetCenter().y);
	auto pointer = [&](InputEventType type, Vector2 position, bool pressed)
	{
		InputEvent event;
		event.m_type = type;
		event.m_position = position;
		event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
		event.m_pressed = pressed;
		editor->m_editorUI->HandleInput(event);
	};
	auto startDrag = [&]()
	{
		pointer(InputEventType::MouseButton, sourcePosition, true);
		pointer(InputEventType::MouseMotion, sourcePosition + Vector2(12.f, 0.f), false);
		app.RunFrame();
		field = FindScriptControl<Button>(editor->m_editorUI->GetRootControl(), [](Button* b) { return b->GetName() == String("ScriptResource"); });
		if (field != nullptr)
		{
			pointer(InputEventType::MouseMotion, field->GetGlobalPosition() + field->GetSize() * 0.5f, false);
		}

		check(viewport->IsGuiDragging() && viewport->GetGuiDragTarget() == field, "FileSystem drag reaches only the Script resource button");
	};
	startDrag();
	capture("script-drag");
	InputEvent cancelDrag;
	cancelDrag.m_type = InputEventType::Key;
	cancelDrag.m_keyCode = GLFW_KEY_ESCAPE;
	cancelDrag.m_pressed = true;
	editor->m_editorUI->HandleInput(cancelDrag);
	app.RunFrame();
	check(!viewport->IsGuiDragging() && moving->GetScriptInstance() == nullptr && !editor->IsSceneDirty(),
		"Escape cancels script drag without assigning or dirtying");
	startDrag();
	if (field != nullptr)
	{
		pointer(InputEventType::MouseButton, field->GetGlobalPosition() + field->GetSize() * 0.5f, false);
	}

	check(moving->GetScriptInstance() != nullptr && editor->IsSceneDirty(), "FileSystem drop commits script attachment");
	editor->SaveScene();
	for (float scale : { 1.f, 1.25f, 1.5f })
	{
		editor->SetEditorScale(scale);
		app.RunFrame();
		std::string const suffix = std::to_string(static_cast<int>(scale * 100.f));
		capture("inspector-" + suffix);
		auto* scriptMenu = FindScriptControl<Button>(editor->m_editorUI->GetRootControl(), [](Button* b) { return b->GetName() == String("ScriptMenu"); });
		auto& inspector = editor->m_editorUI->GetPanel<InspectorPanel>();
		check(scriptMenu != nullptr && scriptMenu->GetGlobalPosition().x + scriptMenu->GetSize().x
			<= inspector.GetGlobalPosition().x + inspector.GetSize().x, "Script dropdown stays inside Inspector at current scale");
		editor->m_editorUI->CreateScript("res://Scripts", "Node3D", {});
		capture("create-script-" + suffix);
		auto* parent = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup());
		auto* inherits = parent != nullptr ? FindScriptControl<OptionButton>(parent, [](OptionButton*) { return true; }) : nullptr;
		check(inherits != nullptr, "create script exposes bound base types");
		if (inherits != nullptr)
		{
			inherits->OpenOptions();
			check(parent->IsVisible() && viewport->GetGuiPopup() != parent, "Inherits dropdown keeps creation dialog open");
			inherits->CloseOptions();
			check(viewport->GetGuiPopup() == parent, "Inherits selection returns to creation dialog");
		}

		if (parent != nullptr)
		{
			parent->Close();
		}

		EditorUI::MenuItem create;
		create.m_text = "Create New";
		bool menuAction = false;
		create.m_children = { { "Folder...", []() {} }, { "Scene...", []() {} }, { "Script...", [&menuAction]() { menuAction = true; } } };
		editor->m_editorUI->ShowMenu(editor->m_editorUI->GetRootControl(), Vector2(1250.f, 800.f), { create });
		auto* menu = viewport->GetGuiPopup();
		InputEvent key;
		key.m_type = InputEventType::Key;
		key.m_pressed = true;
		key.m_keyCode = GLFW_KEY_RIGHT;
		menu->GuiInput(key);
		check(FindScriptControl<Button>(menu, [](Button* b) { return b->GetText() == String("Script..."); }) != nullptr,
			"Right opens Create New submenu");
		capture("create-menu-" + suffix);
		key.m_keyCode = GLFW_KEY_ESCAPE;
		menu->GuiInput(key);
		check(viewport->GetGuiPopup() == menu, "Escape from submenu preserves parent menu");
		menu->GuiInput(key);
		check(viewport->GetGuiPopup() == nullptr, "Escape from parent closes menu");
		editor->m_editorUI->ShowMenu(editor->m_editorUI->GetRootControl(), Vector2(300.f, 500.f), { create });
		menu = viewport->GetGuiPopup();
		key.m_keyCode = GLFW_KEY_RIGHT;
		menu->GuiInput(key);
		key.m_keyCode = GLFW_KEY_DOWN;
		menu->GuiInput(key);
		menu->GuiInput(key);
		key.m_keyCode = GLFW_KEY_ENTER;
		menu->GuiInput(key);
		key.m_pressed = false;
		menu->GuiInput(key);
		check(menuAction && viewport->GetGuiPopup() == nullptr, "Enter activates submenu action and closes all menus");
		editor->MarkSceneDirty();
		editor->RequestQuit();
		capture("unsaved-" + suffix);
		auto* save = FindScriptControl<Button>(viewport->GetGuiPopup(), [](Button* b) { return b->GetText() == String("Save & Quit"); });
		check(save != nullptr && viewport->GetGuiFocus() == save, "quit dialog focuses Save & Quit");
		if (auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup()))
		{
			popup->Close();
		}

		check(!app.IsQuitting() && editor->IsSceneDirty(), "closing quit dialog cancels quit and retains edits");
	}

	editor->SetEditorScale(1.25f);
	editor->SaveScene();

	auto stop = [&]()
	{
		app.StopGame();
		for (int i = 0; i < 500 && app.IsGameRunning(); ++i)
		{
			app.RunFrame();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}

		check(!app.IsGameRunning(), "Stop releases game process");
	};
	auto play = [&](int speed)
	{
		std::filesystem::remove(project / "progress.txt", ignored);
		editor->RequestPlay();
		for (int i = 0; i < 300; ++i)
		{
			app.RunFrame();
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			if (std::filesystem::exists(project / "progress.txt"))
			{
				break;
			}
		}

		std::string progress;
		std::ifstream(project / "progress.txt") >> progress;
		check(app.IsGameRunning() && progress == std::to_string(speed), "Play compiles, instantiates and runs latest OnProcess");
		check(scripts->GetScriptNativeBase("res://Scripts/Mover.cs") == "Node3D", "source generator maps namespace and subdirectory to native base");
		stop();
	};
	play(1);
	writeScript(2);
	play(2);

	std::ofstream(project / "Scripts/Mover.cs") << "using Ming; public class Mover : Node3D {}";
	check(!scripts->BuildProjectSolution(), "non-partial script produces build diagnostic");
	std::ofstream(project / "Scripts/Mover.cs") << "using Ming; public partial class WrongName : Node3D {}";
	check(!scripts->BuildProjectSolution(), "mismatched filename produces build diagnostic");
	std::ofstream(project / "Scripts/Mover.cs") << "syntax error";
	editor->RequestPlay();
	for (int i = 0; i < 4; ++i)
	{
		app.RunFrame();
	}

	check(!app.IsGameRunning(), "compiler error blocks Play");
	if (auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup()))
	{
		popup->Close();
	}

	writeScript(2, "throw new System.InvalidOperationException(\"CS_RUNTIME_EXCEPTION\");");
	play(2);
	for (int i = 0; i < 4; ++i)
	{
		app.RunFrame();
	}

	check(FindScriptControl<Label>(&editor->m_editorUI->GetPanel<OutputPanel>(), [](Label* label)
		{ return label->GetText().ToUtf8().find("CS_RUNTIME_EXCEPTION") != std::string::npos; }) != nullptr,
		"managed exceptions reach editor Output");
	writeScript(2);
	std::ofstream(project / "Scripts/Missing.cs") << "public class Missing {}";
	root = static_cast<Node3D*>(app.GetSceneTree()->GetScene());
	root->SetScript(Variant(ResourceLoader::Load("res://Scripts/Missing.cs")));
	editor->SaveScene();
	editor->RequestPlay();
	for (int i = 0; i < 4; ++i)
	{
		app.RunFrame();
	}

	check(!app.IsGameRunning(), "missing compiled mapping blocks Play");
	if (auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup()))
	{
		popup->Close();
	}

	root->SetScript(Variant(static_cast<Object*>(nullptr)));
	auto* wrongBase = MemNew<Node>();
	wrongBase->SetScript(Variant(ResourceLoader::Load("res://Scripts/Mover.cs")));
	root->AddNode(wrongBase);
	editor->SaveScene();
	editor->RequestPlay();
	for (int i = 0; i < 4; ++i)
	{
		app.RunFrame();
	}

	check(!app.IsGameRunning(), "incompatible script base blocks Play");
	if (auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup()))
	{
		popup->Close();
	}

	wrongBase->QueueFree();
	app.RunFrame();
	editor->SaveScene();
	{
		std::ifstream input(project / "Scripts/Mover.cs");
		std::string source((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
		source.insert(source.find("float elapsed;"), "public Mover() { throw new System.InvalidOperationException(\"CS_CTOR_FAILURE\"); }\n");
		std::ofstream(project / "Scripts/Mover.cs") << source;
	}

	editor->RequestPlay();
	bool launched = false;
	for (int i = 0; i < 300; ++i)
	{
		app.RunFrame();
		launched = launched || app.IsGameRunning();
		if (launched && !app.IsGameRunning())
		{
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	app.RunFrame();
	check(launched && !app.IsGameRunning(), "constructor failure exits the game instead of running an unscripted scene");
	check(FindScriptControl<Label>(&editor->m_editorUI->GetPanel<OutputPanel>(), [](Label* label)
		{ return label->GetText().ToUtf8().find("CS_CTOR_FAILURE") != std::string::npos; }) != nullptr,
		"constructor failure includes its cause in Output");
	writeScript(2);
	check(scripts->BuildProjectSolution() && scripts->ReloadProjectAssembly(), "fixture leaves a runnable sample");
	auto* incompatible = MemNew<Node>();
	incompatible->SetScript(Variant(ResourceLoader::Load("res://Scripts/Mover.cs")));
	check(!scripts->ValidateSceneScripts(incompatible), "incompatible native script base is rejected");
	MemDelete(incompatible);
	check(scripts->GetScriptNativeBase("res://Scripts/Missing.cs").empty(), "unregistered script mapping is distinguishable");
	app.Shutdown();
	g_app = nullptr;
	std::printf("CSHARP_WORKFLOW failures=%d sample=%s\n", failures, project.u8string().c_str());
	return failures == 0 ? 0 : 1;
}
