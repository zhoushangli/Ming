#pragma once
#include "MingEngine/Editor/BuiltinIcons.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"

namespace NativePanelVerification
{
template <typename T>
T* FindControl(Node* node)
{
	if (auto* control = dynamic_cast<T*>(node))
	{
		return control;
	}

	for (Node* child : node->GetChildren())
	{
		if (T* control = FindControl<T>(child))
		{
			return control;
		}
	}

	return nullptr;
}

inline LineEdit* FindFilter(Node* node, String const& placeholder)
{
	if (auto* edit = dynamic_cast<LineEdit*>(node); edit != nullptr && edit->GetPlaceholder() == placeholder)
	{
		return edit;
	}

	for (Node* child : node->GetChildren())
	{
		if (LineEdit* edit = FindFilter(child, placeholder))
		{
			return edit;
		}
	}

	return nullptr;
}

inline uint64_t FileItem(Tree* tree, String const& path)
{
	for (int row = 0; row < static_cast<int>(tree->GetInternalMinimumSize().y / tree->GetRowHeight()); ++row)
	{
		Vector2 const point(80.f, (row + 0.5f) * tree->GetRowHeight());
		GuiDragData   data;
		if (tree->GetDragData(point, data) && data.m_value == path)
		{
			return tree->GetItemAtPosition(point);
		}
	}

	return 0;
}

inline Button* FindButton(Node* node, String const& text)
{
	if (auto* button = dynamic_cast<Button*>(node); button != nullptr && button->IsVisibleInHierarchy()
													&& !button->IsQueuedForDeletion() && button->GetText() == text)
	{
		return button;
	}

	for (Node* child : node->GetChildren())
	{
		if (Button* button = FindButton(child, text))
		{
			return button;
		}
	}

	return nullptr;
}
}

inline void VerifyNativeEditorPanels(
	Engine& engine, std::function<void(bool, char const*)> const& check, std::filesystem::path const& projectPath)
{
	using namespace NativePanelVerification;
	std::filesystem::path const fixture = std::filesystem::weakly_canonical(projectPath / "PanelMigration");
	if (fixture.parent_path() != std::filesystem::weakly_canonical(projectPath))
	{
		check(false, "panel fixture stays inside its verification project");
		return;
	}

	std::filesystem::remove_all(fixture);
	std::filesystem::create_directories(fixture);
	{
		std::ofstream scene(projectPath / "PanelMigration" / "scene.tscn");
		scene << R"({"nodes":[{"name":"PanelFixture","type":"Node3D","parent":""}]})";
		std::ofstream obj(projectPath / "PanelMigration" / "editor-panel-triangle.obj");
		obj << "v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nvn 0 0 1\nf 1/1/1 2/2/1 3/3/1\n";
	}

	SceneTree tree;
	auto*     root = static_cast<Viewport*>(tree.GetRoot());
	root->UpdateResolution(IntVec2(1100, 700));
	auto* editor = MemNew<EditorNode>();
	root->AddNode(editor);
	EditorUI* ui = editor->m_editorUI;
	ui->ResetNativeLayout();
	check(editor->LoadScene("res://PanelMigration/scene.tscn"), "panel migration fixture loads its own scene");
	auto frame = [&]()
	{
		engine.BeginFrame();
		tree.UpdateScene(0.016f);
		engine.m_renderServer->Render();
		engine.EndFrame();
		tree.FlushPendingNode();
	};
	frame();
	frame();
	check(
		ui->GetDockSlot(0)->GetCurrentPage() == &ui->GetPanel<ScenePanel>()
			&& ui->GetDockSlot(1)->GetCurrentPage() == &ui->GetPanel<FileSystemPanel>()
			&& ui->GetDockSlot(2)->GetCurrentPage() == &ui->GetPanel<InspectorPanel>()
			&& ui->GetDockSlot(3)->GetCurrentPage() == &ui->GetPanel<OutputPanel>(),
		"Warm Carbon opens Scene, FileSystem, Inspector and Output in their default regions");
	bool iconsValid = static_cast<bool>(BuiltinIcons::Find("Dropdown"));
	for (auto const& icon : BuiltinIcons::Detail::kEntries)
	{
		auto texture = EditorIcons::GetIconTexture(std::string(icon.m_name));
		iconsValid   = iconsValid && texture.IsValid() && texture->GetDimensions() == IntVec2(32, 32);
	}

	check(iconsValid, "all Warm Carbon icons load as embedded 32 pixel textures");
	check(
		EditorIcons::GetIconColor("Node3D", ui->GetRootControl()) == Color(166, 139, 212),
		"node type icons retain the theme category color");
	engine.m_fileSystem->WriteText("res://.ming/Editor/NativeEditorLayout.json", R"({"version":1,"slots":[]})");
	ui->LoadNativeLayout();
	std::string layout;
	engine.m_fileSystem->ReadText("res://.ming/Editor/NativeEditorLayout.json", layout);
	check(
		nlohmann::json::parse(layout).value("version", 0) == 2
			&& ui->GetDockSlot(1)->GetCurrentPage() == &ui->GetPanel<FileSystemPanel>(),
		"old layout cache is replaced by the new default without migration");
	editor->GetSelection().SetSelected(tree.GetScene()->GetObjectID());
	frame();
	InspectorPanel& inspector         = ui->GetPanel<InspectorPanel>();
	LineEdit*       propertyFilter    = FindControl<LineEdit>(&inspector);
	bool const      dirtyBeforeFilter = editor->IsSceneDirty();
	propertyFilter->SetText("position");
	frame();
	bool visibleField = false;
	bool hiddenField  = false;
	for (Node* child : inspector.GetContentControl()->GetChildren())
	{
		if (auto* field = dynamic_cast<VBoxContainer*>(child))
		{
			visibleField = visibleField || field->IsVisible();
			hiddenField  = hiddenField || !field->IsVisible();
		}
	}

	check(
		visibleField && hiddenField && editor->IsSceneDirty() == dirtyBeforeFilter,
		"Inspector property filtering hides unrelated fields without dirtying the scene");
	propertyFilter->SetText("");
	frame();
	auto click = [&](char const* text)
	{
		Node*   search = root->GetGuiPopup() != nullptr ? static_cast<Node*>(root->GetGuiPopup()) : root;
		Button* button = FindButton(search, text);
		if (button == nullptr || button->IsDisabled())
		{
			std::printf("MISSING_NATIVE_ACTION %s\n", text);
		}
		check(button != nullptr && !button->IsDisabled(), "native action is present and enabled");
		if (button == nullptr || button->IsDisabled())
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
		return true;
	};
	auto key = [&](int code, int modifiers = 0)
	{
		InputEvent event;
		event.m_type      = InputEventType::Key;
		event.m_keyCode   = code;
		event.m_modifiers = modifiers;
		event.m_pressed   = true;
		root->ProcessInputEvent(event);
		event.m_pressed = false;
		root->ProcessInputEvent(event);
	};
	auto rightClickSelected = [&]()
	{
		Tree* files = ui->GetFileTreeControl();
		files->EnsureSelectedVisible();
		frame();
		InputEvent event;
		event.m_type             = InputEventType::MouseButton;
		event.m_mouseButton      = GLFW_MOUSE_BUTTON_RIGHT;
		AABB2 const   row        = files->GetItemRect(files->GetSelected());
		auto*         scroll     = static_cast<ScrollContainer*>(files->GetParent());
		Vector2 const top        = files->GetGlobalPosition() + row.m_mins;
		float const   visibleTop = std::max(top.y, scroll->GetGlobalPosition().y);
		float const   visibleBottom =
			std::min(top.y + row.GetHeight(), scroll->GetGlobalPosition().y + scroll->GetPageSize().y);
		event.m_position = Vector2(top.x + std::min(70.f, row.GetWidth() * 0.5f), (visibleTop + visibleBottom) * 0.5f);
		event.m_pressed  = true;
		root->ProcessInputEvent(event);
		event.m_pressed = false;
		root->ProcessInputEvent(event);
		frame();
	};

	auto* gameLabel = MemNew<Label>();
	tree.GetScene()->AddNode(gameLabel);
	gameLabel->SetText("Game UI");
	Label*    editorLabel    = FindControl<Label>(&ui->GetPanel<InspectorPanel>());
	int const editorFontSize = editor->GetEditorTheme()->GetDefaultFontSize();
	check(
		editorLabel != nullptr && editorLabel->GetFontSize() == editorFontSize
			&& gameLabel->GetFontSize() == ThemeDatabase::GetDefaultTheme()->GetDefaultFontSize(),
		"production editor and game UI use isolated default fonts and sizes");
	editor->GetEditorTheme()->SetDefaultFontSize(14);
	check(
		editorLabel->GetFontSize() == 14
			&& gameLabel->GetFontSize() == ThemeDatabase::GetDefaultTheme()->GetDefaultFontSize(),
		"live editor font change leaves game UI unchanged");
	editor->GetEditorTheme()->SetDefaultFontSize(editorFontSize);
	gameLabel->QueueFree();
	frame();

	if (!click("Project") || !click("Project Settings..."))
	{
		return;
	}

	check(
		root->GetGuiPopup() != nullptr && root->GetGuiPopup()->GetThemeFontSize("font_size") == editorFontSize,
		"native project settings popup uses editor Context");
	VirtualPath const previousStart = ProjectSettings::Get()->m_startScenePath;
	LineEdit*         settingsField = FindControl<LineEdit>(root->GetGuiPopup());
	settingsField->SetText("res://discarded.tscn");
	key(GLFW_KEY_TAB);
	Control* focus       = root->GetGuiFocus();
	bool     withinPopup = false;
	for (Node* node = focus; node != nullptr; node = node->GetParent())
	{
		withinPopup = withinPopup || node == root->GetGuiPopup();
	}
	check(withinPopup, "Tab focus remains inside native modal");
	if (!click("Cancel"))
	{
		return;
	}
	check(ProjectSettings::Get()->m_startScenePath == previousStart, "native settings Cancel discards draft");

	{
		auto* fileTabs = dynamic_cast<TabContainer*>(ui->GetPanel<FileSystemPanel>().GetParent());
		fileTabs->SetCurrentTab(fileTabs->GetPageIndex(&ui->GetPanel<FileSystemPanel>()));
	}
	LineEdit* filter = FindFilter(&ui->GetPanel<FileSystemPanel>(), "Filter files");
	filter->SetText("PanelMigration");
	frame();
	Tree* files = ui->GetFileTreeControl();
	files->Select(FileItem(files, "res://PanelMigration"), true);
	rightClickSelected();
	if (!click("Create New") || !click("Folder..."))
	{
		return;
	}
	FindControl<LineEdit>(root->GetGuiPopup())->SetText("CreatedFolder");
	if (!click("Create"))
	{
		return;
	}
	check(engine.m_fileSystem->Exists("res://PanelMigration/CreatedFolder"), "native file dialog creates a folder");
	filter->SetText("CreatedFolder");
	frame();
	files->EditSelected();
	key(GLFW_KEY_A, GLFW_MOD_CONTROL);
	for (char character : std::string("RenamedFolder"))
	{
		InputEvent event;
		event.m_type      = InputEventType::Text;
		event.m_codepoint = static_cast<uint32_t>(character);
		root->ProcessInputEvent(event);
	}

	key(GLFW_KEY_ENTER);
	frame();

	check(
		engine.m_fileSystem->Exists("res://PanelMigration/RenamedFolder")
			&& !engine.m_fileSystem->Exists("res://PanelMigration/CreatedFolder"),
		"native file rename uses existing validation and filesystem operation");
	filter->SetText("RenamedFolder");
	frame();
	rightClickSelected();
	if (!click("Duplicate"))
	{
		return;
	}
	VirtualPath const duplicate = ui->GetPanel<FileSystemPanel>().GetSelectedVirtualPath();
	check(
		duplicate != VirtualPath("res://PanelMigration/RenamedFolder") && engine.m_fileSystem->Exists(duplicate),
		"native Duplicate selects the newly created entry");
	rightClickSelected();
	if (!click("Delete...") || !click("Cancel"))
	{
		return;
	}
	check(engine.m_fileSystem->Exists(duplicate), "native deletion cancellation preserves the entry");
	rightClickSelected();
	if (!click("Delete...") || !click("Delete"))
	{
		return;
	}
	check(!engine.m_fileSystem->Exists(duplicate), "native deletion requires confirmation before removing the entry");

	filter->SetText("PanelMigration");
	frame();
	files->Select(FileItem(files, "res://PanelMigration"), true);
	rightClickSelected();
	if (!click("Create New") || !click("Scene..."))
	{
		return;
	}
	LineEdit* name = FindControl<LineEdit>(root->GetGuiPopup());
	name->SetText("Generated");
	for (Node* sibling : name->GetParent()->GetChildren())
	{
		if (auto* edit = dynamic_cast<LineEdit*>(sibling); edit != nullptr && edit != name)
		{
			edit->SetText("RootCreated");
		}
	}
	if (!click("Create"))
	{
		return;
	}
	check(
		engine.m_fileSystem->Exists("res://PanelMigration/Generated.tscn")
			&& tree.GetScene()->GetName() == String("RootCreated"),
		"native scene dialog creates and opens the scene");
	ui->GetPanel<ScenePanel>().OpenCreateNode();
	frame();
	FindControl<LineEdit>(root->GetGuiPopup())->SetText("Node3D");
	Tree* types = FindControl<Tree>(root->GetGuiPopup());
	types->Select(types->GetItemAtPosition(Vector2(80.f, types->GetRowHeight() * 0.5f)), true);
	if (!click("Create"))
	{
		return;
	}
	check(
		!tree.GetScene()->GetChildren().empty() && editor->IsSceneDirty(),
		"native node dialog creates a child and marks scene dirty");
	editor->RequestLoadScene("res://PanelMigration/scene.tscn");
	frame();
	check(
		root->GetGuiPopup() != nullptr && tree.GetScene()->GetName() == String("RootCreated"),
		"native unsaved-scene modal blocks replacement");
	key(GLFW_KEY_ESCAPE);
	frame();
	check(
		root->GetGuiPopup() == nullptr && tree.GetScene()->GetName() == String("RootCreated"),
		"Escape cancels the pending scene operation");
	editor->RequestQuit();
	frame();
	key(GLFW_KEY_ESCAPE);
	frame();
	editor->RequestQuit();
	frame();
	check(root->GetGuiPopup() != nullptr, "cancelled quit can be requested again");
	key(GLFW_KEY_ESCAPE);
	frame();
	check(editor->SaveScene(), "native editor saves the newly created node");

	{
		auto* fileTabs = dynamic_cast<TabContainer*>(ui->GetPanel<FileSystemPanel>().GetParent());
		fileTabs->SetCurrentTab(fileTabs->GetPageIndex(&ui->GetPanel<FileSystemPanel>()));
	}
	filter->SetText("editor-panel-triangle");
	frame();
	files->Select(FileItem(files, "res://PanelMigration/editor-panel-triangle.obj"), true);
	frame();
	ui->GetDockSlot(0)->SetCurrentTab(ui->GetDockSlot(0)->GetPageIndex(&ui->GetPanel<ImportPanel>()));
	frame();
	SpinBox* scale = FindControl<SpinBox>(&ui->GetPanel<ImportPanel>());
	check(
		scale != nullptr && scale->GetValue().As<float>() == 1.f,
		"native import panel preserves unit scale defaults without static initialization dependencies");
	if (!click("On") || !click("Reimport (*)"))
	{
		return;
	}
	std::string                              importer;
	std::unordered_map<std::string, Variant> options;
	check(
		ResourceImporter::TryReadImportConfig("res://PanelMigration/editor-panel-triangle.obj", importer, options)
			&& options.count("Generate Tangents") != 0 && !options.at("Generate Tangents").As<bool>(),
		"native import options are saved by Reimport");
	SaveRenderedFrame(
		engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
		projectPath.parent_path() / "Captures" / "editor-native-panels.ppm");
	if (!click("View") || !click("Import"))
	{
		return;
	}
	check(!ui->GetPanel<ImportPanel>().IsOpen(), "View menu closes a native panel");
	if (!click("View") || !click("Import"))
	{
		return;
	}
	check(ui->GetPanel<ImportPanel>().IsOpen(), "View menu reopens the existing native panel");
}
