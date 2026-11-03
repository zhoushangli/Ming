#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/Resource/MaterialResource.hpp"
#include "MingEngine/Scene/3D/MeshInstance3D.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <cstdio>
#include <fstream>
#include <chrono>
#include <cmath>
#include <share.h>
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/SplitContainer.hpp"

template <typename T, typename Predicate>
T* FindWorkflowControl(Node* node, Predicate predicate)
{
	if (node == nullptr)
	{
		return nullptr;
	}
	if (auto* value = dynamic_cast<T*>(node);
		value != nullptr && !value->IsQueuedForDeletion() && value->IsVisibleInHierarchy() && predicate(value))
	{
		return value;
	}
	for (Node* child : node->GetChildren())
	{
		if (auto* value = FindWorkflowControl<T>(child, predicate))
		{
			return value;
		}
	}
	return nullptr;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		return 2;
	}
	int  checks   = 0;
	int  failures = 0;
	auto check    = [&](bool value, char const* message)
	{
		++checks;
		if (!value)
		{
			++failures;
			std::printf("FAIL: %s\n", message);
			std::fflush(stdout);
		}
	};
	auto project = std::filesystem::absolute(argv[1])
				   / ("Workflow-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::create_directories(project);
	auto userData = project / L"UserData";
	std::filesystem::create_directories(userData);
	_wputenv_s(L"LOCALAPPDATA", userData.c_str());
	EngineConfig config;
	config.m_fileSystemConfig.m_resourceRoot = project;
	config.m_scriptSystemConfig.m_isEnabled  = false;
	config.m_audioConfig.m_isEnable          = false;
	config.m_networkConfig.m_isEnable        = false;
	Engine engine(config);
	ClassDatabase::Startup();
	RegisterAllTypes();
	engine.Startup();
	std::ofstream(project / L"project.ming") << R"({"type":"ProjectSettings","version":1,"start_scene_path":""})";
	std::string settingsError;
	check(ProjectSettings::Load(project, settingsError), "fixture installs its own project settings");
	auto* window = engine.m_windowSystem->GetGLFWWindow();
	glfwSetWindowFocusCallback(window, nullptr);
	glfwSetKeyCallback(window, nullptr);
	glfwSetMouseButtonCallback(window, nullptr);
	glfwSetCursorPosCallback(window, nullptr);
	glfwSetCharCallback(window, nullptr);
	glfwSetScrollCallback(window, nullptr);
	glfwSetFramebufferSizeCallback(window, nullptr);
	{
		SceneTree tree;
		auto*     root = static_cast<Viewport*>(tree.GetRoot());
		root->UpdateResolution(IntVec2(1440, 900));
		auto* editor = MemNew<EditorNode>();
		root->AddNode(editor);
		auto* ui    = editor->m_editorUI;
		auto  frame = [&]()
		{
			engine.BeginFrame();
			tree.UpdateScene(0.016f);
			engine.m_renderServer->Render();
			engine.EndFrame();
			tree.FlushPendingNode();
		};
		auto click = [&](char const* text)
		{
			Node* scope =
				root->GetGuiPopup() != nullptr ? static_cast<Node*>(root->GetGuiPopup()) : ui->GetRootControl();
			auto* button = FindWorkflowControl<Button>(
				scope,
				[&](Button* candidate)
				{
					return candidate->GetText() == String(text);
				});
			check(button != nullptr, text);
			if (button == nullptr)
			{
				return false;
			}
			InputEvent event;
			event.m_type        = InputEventType::MouseButton;
			event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
			event.m_position    = button->GetGlobalPosition() + button->GetSize() * 0.5f;
			event.m_pressed     = true;
			root->ProcessInputEvent(event);
			event.m_pressed = false;
			root->ProcessInputEvent(event);
			frame();
			frame();
			return true;
		};
		auto filename = [&](char const* name)
		{
			auto* edit = FindWorkflowControl<LineEdit>(
				root->GetGuiPopup(),
				[](LineEdit* value)
				{
					return value->GetName() == String("SceneFilename");
				});
			check(edit != nullptr, "scene picker exposes its filename field");
			if (edit != nullptr)
			{
				edit->SetText(name);
			}
		};
		frame();
		ui->ResetNativeLayout();
		frame();
		check(
			ui->GetDockSlot(0)->GetPageIndex(&ui->GetPanel<ImportPanel>()) >= 0
				&& ui->GetDockSlot(2)->GetCurrentPage() == &ui->GetPanel<InspectorPanel>(),
			"Godot default places Import with Scene and Inspector on the right");
		click("Node3D");
		check(
			editor->HasScene() && editor->IsSceneDirty() && !editor->GetCurrentScenePath().IsValid(),
			"root creation produces an unsaved scene");
		check(!editor->SaveScene(), "first save opens the scene picker");
		frame();
		filename("first");
		click("Save");
		check(
			editor->GetCurrentScenePath() == VirtualPath("res://first.tscn") && !editor->IsSceneDirty(),
			"first save assigns the extension and path after success");
		check(
			editor->SetMainScene("res://first.tscn") && !editor->SetMainScene("res://missing.tscn"),
			"main scene validates the resource before writing settings");
		editor->MarkSceneDirty();
		editor->RequestNewScene();
		frame();
		click("Cancel");
		check(editor->HasScene() && editor->IsSceneDirty(), "cancelling new scene preserves edits");
		editor->RequestNewScene();
		frame();
		click("Don't Save");
		check(
			!editor->HasScene() && !editor->GetCurrentScenePath().IsValid(),
			"discarding into a new scene clears the previous identity");
		click("Control");
		check(!editor->GetCurrentScenePath().IsValid(), "new root cannot reuse the previous save path");
		editor->RequestCloseScene();
		frame();
		click("Save & Continue");
		frame();
		click("Cancel");
		check(editor->HasScene() && editor->IsSceneDirty(), "cancelling first save cancels deferred close");
		editor->RequestCloseScene();
		frame();
		click("Save & Continue");
		frame();
		filename("second");
		click("Save");
		frame();
		check(
			!editor->HasScene() && std::filesystem::exists(project / L"second.tscn"),
			"successful first save resumes deferred close");
		check(editor->LoadScene("res://first.tscn"), "saved scene loads again");
		frame();
		std::filesystem::create_directory(project / L"blocked.tscn");
		editor->MarkSceneDirty();
		check(
			!editor->SaveSceneAs("res://blocked.tscn")
				&& editor->GetCurrentScenePath() == VirtualPath("res://first.tscn") && editor->IsSceneDirty(),
			"failed save retains path and dirty state");
		editor->RequestSaveSceneAs();
		frame();
		filename("second.tscn");
		click("Save");
		check(
			root->GetGuiPopup() != nullptr && editor->GetCurrentScenePath() == VirtualPath("res://first.tscn"),
			"overwrite requires explicit second confirmation");
		click("Overwrite");
		check(
			editor->GetCurrentScenePath() == VirtualPath("res://second.tscn"),
			"confirmed save as updates the current path");
		std::filesystem::rename(project / L"second.tscn", project / L"renamed.tscn");
		editor->FilesMoved("res://second.tscn", "res://renamed.tscn");
		check(
			editor->SaveScene() && editor->GetCurrentScenePath() == VirtualPath("res://renamed.tscn")
				&& !std::filesystem::exists(project / L"second.tscn"),
			"saving after rename uses the updated path");
		Node* scene  = tree.GetScene();
		auto* first  = MemNew<Node3D>();
		auto* second = MemNew<Node3D>();
		scene->AddNode(first);
		scene->AddNode(second);
		scene->MoveChild(second, 0);
		check(
			scene->GetChildren().front() == second && second->GetParent() == scene,
			"MoveChild preserves parent membership");
		editor->GetSelection().SetSelected(first->GetObjectID());
		frame();
		auto* field = FindWorkflowControl<SpinBox>(
			&ui->GetPanel<InspectorPanel>(),
			[](SpinBox*)
			{
				return true;
			});
		check(field != nullptr, "Inspector exposes editable node properties");
		if (field != nullptr)
		{
			editor->GetUndoRedo().ClearHistory(scene->GetObjectID());
			field->GetLineEdit()->GrabFocus();
			InputEvent input;
			input.m_type    = InputEventType::Key;
			input.m_keyCode = GLFW_KEY_ENTER;
			input.m_pressed = true;
			field->GetLineEdit()->SelectAll();
			InputEvent text;
			text.m_type      = InputEventType::Text;
			text.m_codepoint = '4';
			root->ProcessInputEvent(text);
			field->GetLineEdit()->GuiInput(input);
			check(editor->GetUndoRedo().HasUndo(scene->GetObjectID()), "Inspector commit records a scene action");
			editor->GetUndoRedo().Undo(scene->GetObjectID());
			check(first->GetLocalPosition() == Vector3::Zero, "Inspector undo restores the previous value");
		}
		auto* meshNode = MemNew<MeshInstance3D>();
		scene->AddNode(meshNode);
		Ref<MeshResource> oldMesh    = CreateRef<MeshResource>();
		ObjectID const    resourceID = oldMesh->GetObjectID();
		meshNode->SetMeshResource(oldMesh);
		auto& undo = editor->GetUndoRedo();
		undo.ClearHistory(scene->GetObjectID());
		undo.CreateAction("Clear Mesh", scene->GetObjectID());
		undo.AddDoProperty(meshNode, "mesh", Variant(static_cast<Object*>(nullptr)));
		undo.AddUndoProperty(meshNode, "mesh", Variant(oldMesh));
		check(undo.CommitAction(), "resource property change commits");
		oldMesh = nullptr;
		check(ObjectDatabase::GetInstance(resourceID) != nullptr, "undo history retains the previous resource");
		undo.Undo(scene->GetObjectID());
		{
			Ref<MeshResource> restored(meshNode->GetMeshResource());
			check(
				restored.IsValid() && restored->GetObjectID() == resourceID,
				"resource undo restores the retained reference");
		}
		undo.Redo(scene->GetObjectID());
		undo.ClearHistory(scene->GetObjectID());
		check(ObjectDatabase::GetInstance(resourceID) == nullptr, "clearing history releases unused resources");
		OutputPanel& output   = ui->GetPanel<OutputPanel>();
		size_t const messages = output.GetMessageCount();
		INFO_PRINT("Workflow log delivery");
		frame();
		check(output.GetMessageCount() > messages, "reported messages reach Output on the next editor frame");

		auto press = [&](char const* name)
		{
			auto* button = FindWorkflowControl<Button>(
				&output,
				[&](Button* value)
				{
					return value->GetName() == String(name);
				});
			check(button != nullptr, name);
			if (button == nullptr)
			{
				return;
			}
			InputEvent event;
			event.m_type        = InputEventType::MouseButton;
			event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
			event.m_position    = button->GetGlobalPosition() + button->GetSize() * 0.5f;
			event.m_pressed     = true;
			root->ProcessInputEvent(event);
			event.m_pressed = false;
			root->ProcessInputEvent(event);
			frame();
			frame();
		};
		auto labelCount = [&](String const& text)
		{
			int                        result = 0;
			std::function<void(Node*)> visit  = [&](Node* node)
			{
				if (auto* label = dynamic_cast<Label*>(node); label != nullptr && !label->IsQueuedForDeletion()
															  && label->IsVisibleInHierarchy()
															  && label->GetText() == text)
				{
					++result;
				}
				for (Node* child : node->GetChildren())
				{
					visit(child);
				}
			};
			visit(&output);
			return result;
		};
		output.ClearMessages();
		output.AddMessage({ "", OutputPanel::Level::Info, "Adjacent" });
		output.AddMessage({ "", OutputPanel::Level::Warning, "Boundary" });
		output.AddMessage({ "", OutputPanel::Level::Info, "Adjacent" });
		output.AddMessage({ "", OutputPanel::Level::Info, "Adjacent" });
		frame();
		frame();
		press("CombineOutput");
		press("StatusWarning");
		check(
			labelCount("Adjacent") == 1 && labelCount("Adjacent (2)") == 1 && labelCount("Boundary") == 0,
			"Output filtering never combines across a hidden intervening message");
		output.ClearMessages();
		output.AddMessage({ "", OutputPanel::Level::Warning, "Still filtered" });
		frame();
		frame();
		check(labelCount("Still filtered") == 0, "clearing Output preserves independent level filters");
		press("StatusWarning");
		output.ClearMessages();
		for (int i = 0; i < 80; ++i)
		{
			output.AddMessage({ "", OutputPanel::Level::Info, "Scroll line " + std::to_string(i) });
		}
		frame();
		frame();
		auto* outputScroll = FindWorkflowControl<ScrollContainer>(
			&output,
			[](ScrollContainer*)
			{
				return true;
			});
		check(
			outputScroll != nullptr && outputScroll->GetContentSize().y > outputScroll->GetPageSize().y,
			"Output scrolling fixture prepares its own overflow");
		if (outputScroll != nullptr)
		{
			outputScroll->SetScroll(0.f);
			output.AddMessage({ "", OutputPanel::Level::Info, "Keep reading" });
			frame();
			frame();
			check(outputScroll->GetScroll() == 0.f, "new messages preserve a reader's scroll position");
			outputScroll->SetScroll(outputScroll->GetContentSize().y);
			output.AddMessage({ "", OutputPanel::Level::Info, "Follow bottom" });
			frame();
			frame();
			check(
				outputScroll->GetScroll() >= outputScroll->GetContentSize().y - outputScroll->GetPageSize().y - 1.f,
				"Output follows new messages only when already at the bottom");
		}
		ui->ToggleOutputCollapsed();
		frame();
		check(output.IsCollapsed(), "Output collapses without closing the panel");
		ui->ToggleOutputCollapsed();
		frame();
		check(!output.IsCollapsed(), "Output restores its content");

		std::vector<std::pair<ObjectID, Vector2>> minimums;
		std::function<void(Node*)>                collectMinimums = [&](Node* node)
		{
			if (node == ui->GetSceneView())
			{
				return;
			}
			if (auto* control = dynamic_cast<Control*>(node); control != nullptr && !control->IsQueuedForDeletion())
			{
				minimums.emplace_back(control->GetObjectID(), control->GetCustomMinimumSize());
			}
			for (Node* child : node->GetChildren())
			{
				collectMinimums(child);
			}
		};
		collectMinimums(ui->GetRootControl());
		float const split = ui->GetDockSplit(0)->GetSplitRatio();
		for (float scale : { 2.f, 1.f, 1.5f, 1.25f, 1.f, 2.f, 1.25f })
		{
			check(editor->SetEditorScale(scale), "supported editor scale applies");
			frame();
			frame();
		}
		bool stable = true;
		for (auto const& item : minimums)
		{
			auto* control = ObjectDatabase::GetInstance<Control>(item.first);
			stable        = stable && control != nullptr && control->GetCustomMinimumSize() == item.second;
		}
		check(
			stable && ui->GetDockSplit(0)->GetSplitRatio() == split,
			"four-scale round trips preserve controls, minimum sizes and user split ratios");
		check(
			editor->SetEditorScale(1.f) && EditorThemeManager::LoadScale() == 1.f,
			"editor scale applies and persists");
		frame();
		check(
			editor->SetEditorScale(1.25f) && EditorThemeManager::LoadScale() == 1.25f,
			"editor scale returns to the default without changing the scene");
		frame();
		std::string error;
		auto        emptyProject = project / std::filesystem::u8path(u8"空项目 with spaces");
		check(
			App::CreateEmptyProject(emptyProject, error) && ProjectSettings::Read(emptyProject, error).IsValid(),
			"empty project creation supports Unicode and spaces");
		check(
			!App::CreateEmptyProject(emptyProject, error) && std::filesystem::exists(emptyProject / L"project.ming"),
			"creation rejects nonempty directories without overwriting");
		std::ofstream(emptyProject / L"project.ming") << "invalid settings";
		check(!ProjectSettings::Read(emptyProject, error).IsValid(), "malformed project reports failure");
		std::ifstream retained(emptyProject / L"project.ming");
		std::string   text;
		std::getline(retained, text);
		check(text == "invalid settings", "project validation does not overwrite malformed files");
		for (int i = 0; i < 12; ++i)
		{
			App::RememberProject(project / ("Recent" + std::to_string(i)));
		}
		App::RememberProject(project / L"recent11");
		check(
			App::LoadRecentProjects().size() == 10 && App::LoadRecentProjects().front().filename() == L"recent11",
			"recent projects deduplicate Windows paths and retain ten entries");
		// 1) Preserve an edited scene and its main-scene identity when a directory moves.
		std::filesystem::create_directory(project / L"BeforeMove");
		check(
			editor->SaveSceneAs("res://BeforeMove/main.tscn") && editor->SetMainScene("res://BeforeMove/main.tscn"),
			"directory move fixture saves its own scene and main setting");
		tree.GetScene()->SetName("UnsavedAfterMove");
		editor->MarkSceneDirty();
		std::filesystem::rename(project / L"BeforeMove", project / L"AfterMove");
		std::filesystem::rename(project / L"project.ming", project / L"project.ming.backup");
		std::filesystem::create_directory(project / L"project.ming");
		editor->FilesMoved("res://BeforeMove", "res://AfterMove");
		check(
			editor->GetCurrentScenePath() == VirtualPath("res://AfterMove/main.tscn")
				&& ProjectSettings::Get()->m_startScenePath == VirtualPath("res://AfterMove/main.tscn")
				&& editor->IsSceneDirty() && tree.GetScene()->GetName() == String("UnsavedAfterMove"),
			"failed settings write preserves moved paths and unsaved scene contents in memory");
		InputEvent escape;
		escape.m_type    = InputEventType::Key;
		escape.m_keyCode = GLFW_KEY_ESCAPE;
		escape.m_pressed = true;
		root->ProcessInputEvent(escape);
		frame();
		std::filesystem::remove(project / L"project.ming");
		std::filesystem::rename(project / L"project.ming.backup", project / L"project.ming");
		check(
			editor->SaveScene() && editor->SetMainScene(editor->GetCurrentScenePath()),
			"saving after a directory move writes the new location and can retry project settings");

		// 2) A failed first save cancels its continuation even when a later correction saves successfully.
		editor->RequestNewScene();
		frame();
		click("Node3D");
		FILE* locked = _wfsopen((project / L"locked.tscn").c_str(), L"w", _SH_DENYWR);
		check(locked != nullptr, "first-save failure fixture holds its own target file");
		bool closed = false;
		editor->RequestCloseScene(
			[&]()
			{
				closed = true;
			});
		frame();
		click("Save & Continue");
		filename("locked.tscn");
		click("Save");
		click("Overwrite");
		check(
			editor->HasScene() && editor->IsSceneDirty() && !closed,
			"write failure preserves the unsaved scene and cancels deferred close");
		if (locked != nullptr)
		{
			std::fclose(locked);
		}
		filename("recovered.tscn");
		click("Save");
		check(
			editor->HasScene() && !editor->IsSceneDirty() && !closed,
			"a later successful correction cannot revive the cancelled close action");

		// 3) Root deletion uses the same cancellation and scene-state cleanup as Close Scene.
		editor->MarkSceneDirty();
		editor->GetSelection().SetSelected(tree.GetScene()->GetObjectID());
		frame();
		ui->GetSceneTreeControl()->GrabFocus();
		InputEvent remove;
		remove.m_type    = InputEventType::Key;
		remove.m_keyCode = GLFW_KEY_DELETE;
		remove.m_pressed = true;
		root->ProcessInputEvent(remove);
		frame();
		click("Cancel");
		check(
			editor->HasScene() && editor->GetCurrentScenePath() == VirtualPath("res://recovered.tscn"),
			"cancelling root deletion preserves the current scene identity");
		ui->GetSceneTreeControl()->GrabFocus();
		root->ProcessInputEvent(remove);
		frame();
		click("Don't Save");
		check(
			!editor->HasScene() && !editor->GetCurrentScenePath().IsValid()
				&& !editor->GetSelection().GetSelected().IsValid(),
			"confirmed root deletion clears scene path and selection");

		Ref<ProjectSettings> parsed =
			ProjectSettings::Parse(R"({"type":"ProjectSettings","version":99,"start_scene_path":""})", error);
		check(
			!parsed.IsValid() && error.find("version") != std::string::npos,
			"unsupported project versions report a distinct error");
	}
	ProjectSettings::Reset();
	ThemeDatabase::Shutdown();
	ResourceLoader::Shutdown();
	EditorIcons::Shutdown();
	engine.Shutdown();
	ClassDatabase::Shutdown();
	std::printf("EDITOR_WORKFLOW checks=%d failures=%d\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
