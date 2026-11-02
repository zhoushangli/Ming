#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/MenuBar.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/GUI/SplitContainer.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyFloat.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "RenderReadback.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <thread>
#include <chrono>
#include <iterator>

template <typename T>
T* FindUIControl(Node* node)
{
	if (auto* result = dynamic_cast<T*>(node))
	{
		return result;
	}

	for (Node* child : node->GetChildren())
	{
		if (auto* result = FindUIControl<T>(child))
		{
			return result;
		}
	}

	return nullptr;
}

Control* FindNamedUI(Node* node, String const& name)
{
	if (auto* control = dynamic_cast<Control*>(node); control != nullptr && node->GetName() == name && !node->IsQueuedForDeletion())
	{
		return control;
	}

	for (Node* child : node->GetChildren())
	{
		if (Control* control = FindNamedUI(child, name))
		{
			return control;
		}
	}

	return nullptr;
}

int main(int argc, char** argv)
{
	if (argc < 2 || argc > 4)
	{
		return 2;
	}
	bool preview = false;
	float uiScale = 1.25f;
	for (int i = 2; i < argc; ++i)
	{
		std::string const argument(argv[i]);
		if (argument == "--preview")
		{
			preview = true;
		}
		else if (argument.rfind("--scale=", 0) == 0)
		{
			uiScale = std::stof(argument.substr(8));
		}
		else
		{
			return 2;
		}
	}

	int checks = 0;
	int failures = 0;
	auto check = [&](bool result, char const* message)
	{
		++checks;
		if (!result)
		{
			++failures;
			std::printf("FAIL: %s\n", message);
		}

		std::fflush(stdout);
	};
	std::filesystem::path const project = std::filesystem::absolute(argv[1]);
	std::filesystem::create_directories(project);
	EngineConfig config;
	config.m_fileSystemConfig.m_resourceRoot = project;
	config.m_scriptSystemConfig.m_isEnabled = false;
	config.m_audioConfig.m_isEnable = false;
	config.m_networkConfig.m_isEnable = false;
	config.m_windowConfig.m_appName = "Editor UI Verification";
	config.m_windowConfig.m_isResizable = true;
	config.m_windowConfig.m_isBorderlessFullscreen = false;
	Engine engine(config);
	ClassDatabase::Startup();
	RegisterAllTypes();
	engine.Startup();
	{
		SceneTree tree;
		auto* root = static_cast<Viewport*>(tree.GetRoot());
		root->UpdateResolution(IntVec2(1440, 900));
		auto* scope = MemNew<Control>();
		root->AddNode(scope);
		scope->UpdateSize(Vector2(1440.f, 900.f));
		auto theme = EditorThemeManager::CreateScaledTheme(EditorThemeManager::CreateTheme(), uiScale);
		ThemeDatabase::SetThemeContext(scope, { theme });
		auto pointer = [&](InputEventType type, Vector2 position, bool pressed = false)
		{
			InputEvent event;
			event.m_type = type;
			event.m_position = position;
			event.m_pressed = pressed;
			event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
			root->ProcessInputEvent(event);
		};
		auto key = [&](int code)
		{
			InputEvent event;
			event.m_type = InputEventType::Key;
			event.m_keyCode = code;
			event.m_pressed = true;
			root->ProcessInputEvent(event);
			event.m_pressed = false;
			root->ProcessInputEvent(event);
		};
		auto click = [&](Control* control)
		{
			Vector2 const position = control->GetGlobalPosition() + control->GetSize() * 0.5f;
			pointer(InputEventType::MouseButton, position, true);
			pointer(InputEventType::MouseButton, position);
		};

		auto* bar = MemNew<MenuBar>();
		scope->AddNode(bar);
		bar->SetPosition(Vector2(20.f, 15.f));
		int actions = 0;
		bar->AddMenu("First", [&]()
		{
			return std::vector<MenuBar::MenuItem>{ { "Disabled", {} }, { "", {}, "", true }, { "Action", [&]() { ++actions; }, "Ctrl+A", false, true, true } };
		});
		bar->AddMenu("Second", []()
		{
			return std::vector<MenuBar::MenuItem>{ { "Unavailable", {} } };
		});
		bar->UpdateSize(bar->GetCombinedMinimumSize());
		tree.FlushContainerUpdate();
		auto* first = static_cast<Button*>(bar->GetChildren()[0]);
		auto* second = static_cast<Button*>(bar->GetChildren()[1]);
		check(first->GetFontSize() == theme->GetDefaultFontSize() && bar->GetSize().y >= theme->GetConstant("minimum_height", "MenuBar") && first->GetSize().y == bar->GetSize().y,
			"editor menu fonts and title hit areas use configured menu height");
		click(first);
		check(bar->GetActiveMenu() == 0 && root->GetGuiPopup() != nullptr && first->IsPressed(),
			"click opens menu and marks its title active");
		auto* focused = dynamic_cast<Button*>(root->GetGuiFocus());
		check(focused != nullptr && focused->GetText() == String("Action"), "menu initially skips disabled items and separators");
		check(focused != nullptr && FindNamedUI(focused, "Shortcut") != nullptr && FindNamedUI(focused, "Check") != nullptr,
			"menu row has independent shortcut and check columns");
		pointer(InputEventType::MouseMotion, second->GetGlobalPosition() + second->GetSize() * 0.5f);
		check(bar->GetActiveMenu() == 1 && second->IsPressed() && !first->IsPressed(),
			"hover switches an open menu through its popup overlay");
		key(GLFW_KEY_RIGHT);
		check(bar->GetActiveMenu() == 0, "Right wraps to the first menu");
		key(GLFW_KEY_LEFT);
		check(bar->GetActiveMenu() == 1, "Left wraps to the last menu");
		key(GLFW_KEY_ESCAPE);
		check(root->GetGuiPopup() == nullptr && bar->GetActiveMenu() == -1 && bar->HasFocus() && !second->IsPressed(),
			"Escape closes menu, clears active appearance and restores title focus");
		key(GLFW_KEY_LEFT);
		key(GLFW_KEY_DOWN);
		key(GLFW_KEY_ENTER);
		check(actions == 1 && root->GetGuiPopup() == nullptr, "keyboard invokes command once and closes menu");
		click(first);
		pointer(InputEventType::MouseButton, Vector2(1200.f, 800.f), true);
		pointer(InputEventType::MouseButton, Vector2(1200.f, 800.f));
		check(bar->GetActiveMenu() == -1 && !first->IsPressed(), "outside click clears menu active state");
		click(first);
		click(first);
		check(root->GetGuiPopup() == nullptr, "clicking the active title closes its menu");
		click(first);
		InputEvent menuFocus;
		menuFocus.m_type = InputEventType::WindowFocus;
		menuFocus.m_focused = false;
		root->ProcessInputEvent(menuFocus);
		check(root->GetGuiPopup() == nullptr && bar->GetActiveMenu() == -1 && !first->IsPressed(),
			"window focus loss closes the open menu and clears its title state");
		menuFocus.m_focused = true;
		root->ProcessInputEvent(menuFocus);
		click(first);
		bar->QueueFree();
		tree.FlushPendingNode();
		check(root->GetGuiPopup() == nullptr, "deleting an open menu bar closes its detached popup");

		auto* split = MemNew<SplitContainer>();
		scope->AddNode(split);
		split->SetPosition(Vector2(30.f, 120.f));
		split->AddNode(MemNew<Control>());
		split->AddNode(MemNew<Control>());
		split->UpdateSize(Vector2(600.f, 220.f));
		tree.FlushContainerUpdate();
		auto dividerPosition = [&]() { return split->GetGlobalPosition() + split->GetDividerRect().GetCenter(); };
		pointer(InputEventType::MouseMotion, dividerPosition());
		check(engine.m_windowSystem->GetCursorShape() == CursorShape::ResizeHorizontal,
			"horizontal split hover selects the system horizontal resize cursor");
		pointer(InputEventType::MouseButton, dividerPosition(), true);
		pointer(InputEventType::MouseMotion, Vector2(10.f, 100.f));
		check(engine.m_windowSystem->GetCursorShape() == CursorShape::ResizeHorizontal,
			"captured split keeps resize cursor when pointer leaves its divider");
		pointer(InputEventType::MouseButton, Vector2(10.f, 100.f));
		check(engine.m_windowSystem->GetCursorShape() == CursorShape::Arrow, "release outside split restores arrow");
		split->SetVertical(true);
		tree.FlushContainerUpdate();
		pointer(InputEventType::MouseMotion, dividerPosition());
		check(engine.m_windowSystem->GetCursorShape() == CursorShape::ResizeVertical,
			"vertical split hover selects the system vertical resize cursor");
		InputEvent focus;
		focus.m_type = InputEventType::WindowFocus;
		focus.m_focused = false;
		root->ProcessInputEvent(focus);
		check(engine.m_windowSystem->GetCursorShape() == CursorShape::Arrow, "window focus loss restores arrow");
		focus.m_focused = true;
		root->ProcessInputEvent(focus);
		pointer(InputEventType::MouseMotion, dividerPosition());
		split->SetVisible(false);
		check(engine.m_windowSystem->GetCursorShape() == CursorShape::Arrow, "hiding hovered split restores arrow");
		auto* actionTree = MemNew<Tree>();
		scope->AddNode(actionTree);
		actionTree->SetPosition(Vector2(20.f, 400.f));
		actionTree->SetClipText(true);
		int tailClicks = 0;
		actionTree->SetItemActionCallback([&](uint64_t, int) { ++tailClicks; });
		Tree::Item item{ 1, 0, "LongNameThatMustLeaveRoomForTailIcons", true, true };
		item.m_actions.push_back({ EditorIcons::GetIconTexture("Visible"), Color::White, true });
		actionTree->SetItems({ item });
		actionTree->UpdateSize(Vector2(220.f, 35.f));
		tree.FlushContainerUpdate();
		Vector2 const tail = actionTree->GetGlobalPosition() + actionTree->GetItemActionRect(1, 0).GetCenter();
		pointer(InputEventType::MouseButton, tail, true);
		pointer(InputEventType::MouseButton, tail);
		check(tailClicks == 1 && actionTree->GetSelected() == 0, "tree tail action does not select or activate the row");
		GuiDragData tailDrag;
		check(!actionTree->GetDragData(actionTree->GetItemActionRect(1, 0).GetCenter(), tailDrag),
			"tree tail action does not initiate row dragging");
		actionTree->Select(1);
		actionTree->EditSelected();
		check(FindUIControl<LineEdit>(actionTree)->GetLocalPosition().x + FindUIControl<LineEdit>(actionTree)->GetSize().x
			<= actionTree->GetItemActionRect(1, 0).m_mins.x, "rename field reserves the same tail area as drawing and hit testing");
		scope->QueueFree();
		tree.FlushPendingNode();

		auto* editor = MemNew<EditorNode>(uiScale);
		root->AddNode(editor);
		EditorUI* ui = editor->m_editorUI;
		auto frame = [&]()
		{
			engine.BeginFrame();
			tree.UpdateScene(0.016f);
			engine.m_renderServer->Render();
			engine.EndFrame();
			tree.FlushPendingNode();
		};
		frame();
		ui->ResetNativeLayout();
		frame();
		MenuBar* productionBar = FindUIControl<MenuBar>(ui->GetRootControl());
		check(productionBar != nullptr && ui->GetDockSplit(0)->GetGlobalPosition().y == editor->GetEditorTheme()->GetConstant("top_bar_height", "Editor"),
			"production editor uses MenuBar and the configured top bar height");
		check(editor->GetEditorTheme()->GetDefaultFontSize() == theme->GetDefaultFontSize()
			&& ThemeDatabase::GetDefaultTheme()->GetDefaultFontSize() == 16,
			"scaled editor dimensions leave runtime theme unchanged");
		auto const contextThemes = ThemeDatabase::GetThemeContext(ui->GetRootControl());
		check(contextThemes.size() == 1 && contextThemes.front() == editor->GetEditorTheme(),
			"editor context contains only the generated Theme");
		check(ThemeDatabase::GetThemeContext(ui->GetSceneView()->GetSubViewport()).empty(),
			"scene preview remains isolated from editor scaling");
		check(ui->GetDockSlot(3)->GetTabBarRect().m_mins.y >= ui->GetDockSlot(3)->GetPageRect().m_maxs.y,
			"Output tabs and their input geometry sit below its content");
		if (productionBar != nullptr)
		{
			auto menuPixels = [&]()
			{
				std::filesystem::path const path = project.parent_path() / "Captures" / "menu-stability.ppm";
				std::filesystem::create_directories(path.parent_path());
				SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), path);
				std::ifstream input(path, std::ios::binary);
				std::string magic;
				int width = 0;
				int height = 0;
				int maximum = 0;
				input >> magic >> width >> height >> maximum;
				input.get();
				std::vector<char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
				std::vector<char> pixels;
				Vector2 const start = productionBar->GetGlobalPosition();
				Vector2 const end = start + productionBar->GetSize();
				for (int y = std::max(0, static_cast<int>(start.y)); y < std::min(height, static_cast<int>(std::ceil(end.y))); ++y)
				{
					int const left = std::clamp(static_cast<int>(start.x), 0, width);
					int const right = std::clamp(static_cast<int>(std::ceil(end.x)), left, width);
					pixels.insert(pixels.end(), bytes.begin() + (y * width + left) * 3, bytes.begin() + (y * width + right) * 3);
				}

				return pixels;
			};
			pointer(InputEventType::MouseMotion, Vector2::Zero);
			root->SetGuiFocus(nullptr);
			frame();
			frame();
			auto const stablePixels = menuPixels();
			for (int i = 0; i < 12; ++i)
			{
				click(static_cast<Control*>(productionBar->GetChildren()[1]));
				frame();
				auto* next = static_cast<Control*>(productionBar->GetChildren()[2]);
				pointer(InputEventType::MouseMotion, next->GetGlobalPosition() + next->GetSize() * 0.5f);
				frame();
				key(GLFW_KEY_ESCAPE);
				pointer(InputEventType::MouseMotion, Vector2::Zero);
				root->SetGuiFocus(nullptr);
				frame();
			}

			check(!stablePixels.empty() && stablePixels == menuPixels(),
				"menu pixels return unchanged after repeated open, hover switching, close and redraw");
		}
		Tree* files = ui->GetFileTreeControl();
		files->GrabFocus();
		check(files->HasFocus() && files->GetThemeStyleBox("focus")->GetBorderWidth(ThemeStyleBox::Side::Right) == 0.f,
			"editor tree keeps keyboard focus without drawing its focus border");
		if (productionBar != nullptr)
		{
			click(static_cast<Control*>(productionBar->GetChildren()[1]));
			check(root->GetGuiPopup() != nullptr && productionBar->GetActiveMenu() == 1,
				"production Project menu uses the independent MenuBar");
			key(GLFW_KEY_ESCAPE);
		}

		auto white = CreateRef<Image>();
		white->LoadFromRGBA8(IntVec2(2, 2), std::vector<uint8_t>(16, 255));
		auto texture = CreateRef<TextureResource>();
		texture->m_image = white;
		texture->InitGPUResources();
		auto* tintProbe = MemNew<TextureRect>();
		root->AddNode(tintProbe);
		tintProbe->SetTexture(texture);
		tintProbe->SetTint(Color(147, 208, 221));
		tintProbe->SetPosition(Vector2(1400.f, 5.f));
		tintProbe->UpdateSize(Vector2(20.f, 20.f));
		frame();
		check(ReadPixel(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), 1405, 10) == Color(147, 208, 221),
			"TextureRect tint reaches the actual rendered texture");
		tintProbe->SetStretchMode(TextureRect::StretchMode::Tile);
		tintProbe->SetHorizontalFlip(true);
		frame();
		check(ReadPixel(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), 1405, 10) == Color(147, 208, 221),
			"tint also applies to tiled and flipped textures");
		tintProbe->QueueFree();
		frame();

		// 1) Build an independent Inspector selection with class-local and inherited properties
		auto* scene = MemNew<Node3D>();
		scene->SetName("StyleVerification");
		auto* camera = MemNew<Camera3D>();
		camera->SetName("InspectorCamera");
		scene->AddNode(camera);
		tree.ChangeScene(scene);
		tree.FlushPendingNode();
		editor->GetSelection().SetSelected(camera->GetObjectID());
		frame();
		frame();
		InspectorPanel& inspector = ui->GetPanel<InspectorPanel>();
		auto headerFor = [&](char const* name) -> Control*
		{
			for (Node* child : inspector.GetContentControl()->GetChildren())
			{
				auto* control = FindUIControl<PanelContainer>(child);
				if (control != nullptr && control->GetThemeTypeVariation() == "EditorInspectorClassHeader")
				{
					Label* label = FindUIControl<Label>(control);
					if (label != nullptr && label->GetText() == String(name))
					{
						return control;
					}
				}
			}

			return nullptr;
		};
		Control* cameraHeader = headerFor("Camera3D");
		Control* nodeHeader = headerFor("Node3D");
		check(cameraHeader != nullptr && nodeHeader != nullptr
			&& cameraHeader->IsVisibleInHierarchy() && nodeHeader->IsVisibleInHierarchy()
			&& cameraHeader->GetGlobalPosition().y < nodeHeader->GetGlobalPosition().y,
			"Inspector shows class headers from derived class to base class");
		if (cameraHeader != nullptr)
		{
			auto* icon = FindUIControl<TextureRect>(cameraHeader);
			auto* title = FindUIControl<Label>(cameraHeader);
			check(icon != nullptr && icon->GetTint() == EditorIcons::GetIconColor("Camera3D", &inspector),
				"Inspector class icon uses the same category color as Scene");
			Color const changedTint(120, 90, 180);
			editor->GetEditorTheme()->SetColor("t3d", "EditorIcons", changedTint);
			frame();
			check(icon != nullptr && icon->GetTint() == changedTint, "Inspector icon tint follows live Theme edits");
			editor->GetEditorTheme()->SetColor("t3d", "EditorIcons", Color(166, 139, 212));
			frame();
			check(icon != nullptr && icon->GetTexture().IsValid() && title != nullptr
				&& std::abs((icon->GetGlobalPosition().x + title->GetGlobalPosition().x + title->GetSize().x) * 0.5f
					- (cameraHeader->GetGlobalPosition().x + cameraHeader->GetSize().x * 0.5f)) < 1.f,
				"class icon and label form one centered group");
			title->SetText("VeryLongClassNameThatMustNotExpandTheInspectorPanel");
			frame();
			frame();
			check(title->GetGlobalPosition().x + title->GetSize().x <= cameraHeader->GetGlobalPosition().x + cameraHeader->GetSize().x
				&& icon->GetSize().x == theme->GetConstant("icon_size", "Button"), "long class names clip inside the bar without shrinking its icon");
			title->SetText("Camera3D");
			frame();
		}

		Button* transformGroup = nullptr;
		for (Node* child : inspector.GetContentControl()->GetChildren())
		{
			if (auto* button = dynamic_cast<Button*>(child); button != nullptr && button->GetText() == String("Transform"))
			{
				transformGroup = button;
			}
		}

		check(transformGroup != nullptr, "Node3D properties are grouped under Transform");
		if (transformGroup != nullptr)
		{
			click(transformGroup);
			frame();
			LineEdit* search = FindUIControl<LineEdit>(&inspector);
			search->SetText("Transform");
			frame();
			check(transformGroup->IsVisibleInHierarchy() && nodeHeader->IsVisibleInHierarchy(), "group-name search expands matching group");
			search->SetText("");
			frame();
			click(transformGroup);
			frame();
		}

		auto* filter = FindUIControl<LineEdit>(&inspector);
		filter->SetText("position");
		frame();
		check(cameraHeader != nullptr && nodeHeader != nullptr && !cameraHeader->IsVisibleInHierarchy() && nodeHeader->IsVisibleInHierarchy(),
			"property search hides the complete unrelated class header");
		SpinBox* position = FindUIControl<SpinBox>(inspector.GetContentControl());
		for (Node* child : inspector.GetContentControl()->GetChildren())
		{
			auto* control = dynamic_cast<Control*>(child);
			if (control != nullptr && control->IsVisible())
			{
				if (auto* spin = FindUIControl<SpinBox>(control))
				{
					position = spin;
					break;
				}
			}
		}

		if (position != nullptr)
		{
			position->Step(1);
			check(camera->GetLocalPosition().x > 0.f && editor->IsSceneDirty(),
				"inherited Inspector property still writes through its setter");
		}

		filter->SetText("no matching property");
		frame();
		check(cameraHeader != nullptr && nodeHeader != nullptr && !cameraHeader->IsVisibleInHierarchy() && !nodeHeader->IsVisibleInHierarchy(),
			"no search matches leaves no empty class bars");
		filter->SetText("");
		editor->GetSelection().SetSelected(ui->GetPanel<ViewportPanel>().GetObjectID());
		frame();
		check(headerFor("Camera3D") == nullptr, "selection switch discards previous class bars");
		editor->GetSelection().Clear();
		frame();
		check(inspector.GetContentControl()->GetChildren().empty(), "clearing selection removes class bars and fields");
		editor->GetSelection().SetSelected(camera->GetObjectID());
		frame();
		frame();
		SubViewportContainer* sceneView = ui->GetSceneView();
		Vector2 const beforeDockResize = sceneView->GetSize();
		ui->GetDockSplit(1)->SetSplitRatio(ui->GetDockSplit(1)->GetSplitRatio() - 0.05f);
		frame();
		frame();
		check(sceneView->GetSize().x <= beforeDockResize.x
			&& sceneView->GetSubViewport()->GetOutputResolution().x == static_cast<int>(sceneView->GetSize().x),
			"dock resize respects minimum widths and keeps the content texture aligned with ruler margins");
		ui->ResetNativeLayout();
		frame();
		frame();

		check(SpinBox::FormatValue(Variant(0.1f)) == "0.1", "float fields use shortest round-trip representation");
		for (int i = 0; i < 4; ++i)
		{
			TabContainer* slot = ui->GetDockSlot(i);
			check(slot->GetTabInsertIndex(slot->GetTitleActions()->GetLocalPosition() + Vector2(5.f, 5.f)) == -1,
				"dock action area cannot become a tab insertion target");
		}

		OutputPanel& outputPanel = ui->GetPanel<OutputPanel>();
		outputPanel.AddMessage({ "10:24:01", OutputPanel::Level::Info, "Project loaded: StyleVerification" });
		outputPanel.AddMessage({ "10:24:02", OutputPanel::Level::Warning, "Texture uses default import settings" });
		outputPanel.AddMessage({ "10:24:03", OutputPanel::Level::Error, "Preview diagnostic message" });
		frame();
		outputPanel.SetErrorsOnly(true);
		frame();
		check(outputPanel.GetMessageCount() == 3, "Errors filtering leaves source messages intact");
		outputPanel.ClearMessages();
		frame();
		check(outputPanel.GetMessageCount() == 0, "Clear removes messages from Output and Errors");
		outputPanel.SetErrorsOnly(false);
		ui->ToggleOutputCollapsed();
		frame();
		check(outputPanel.IsCollapsed() && ui->GetDockSlot(3)->GetSize().y <= ui->GetDockSlot(3)->GetTabBarRect().GetHeight() + 1.f, "Output collapse reserves only its header");
		ui->ToggleOutputCollapsed();
		frame();
		check(!outputPanel.IsCollapsed(), "Output reopens its content area");
		std::filesystem::create_directories(project / "Scenes");
		std::filesystem::create_directories(project / "Scripts");
		std::filesystem::create_directories(project / "Textures");
		{
			std::ofstream fixture(project / "Scenes" / "Level.tscn");
			fixture << R"({"nodes":[{"name":"Level","type":"Node3D","parent":""}]})";
		}

		{
			std::ofstream fixture(project / "Textures" / "preview-triangle.obj");
			fixture << "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
		}

		engine.m_fileSystem->ScanResourceTree();
		FileSystemPanel& filePanel = ui->GetPanel<FileSystemPanel>();
		filePanel.NavigateTo("res://Scenes");
		frame();
		check(filePanel.GetDirectory() == VirtualPath("res://Scenes"), "path navigation changes the displayed directory");
		filePanel.NavigateTo("res://MissingDirectory");
		check(filePanel.GetDirectory() == VirtualPath("res://Scenes"), "invalid directory leaves browsing state intact");
		filePanel.NavigateTo("res://");
		frame();
		click(FindNamedUI(&filePanel, "Back"));
		frame();
		check(filePanel.GetDirectory() == VirtualPath("res://Scenes"), "Back restores the previous directory");
		click(FindNamedUI(&filePanel, "Forward"));
		frame();
		check(filePanel.GetDirectory() == VirtualPath("res://"), "Forward restores the next directory");
		outputPanel.AddMessage({ "10:24:01", OutputPanel::Level::Info, "Project loaded: StyleVerification" });
		outputPanel.AddMessage({ "10:24:02", OutputPanel::Level::Warning, "Preview resource has default import settings" });
		outputPanel.AddMessage({ "10:24:03", OutputPanel::Level::Error, "Preview diagnostic message" });
		for (int i = 0; i < 5; ++i)
		{
			auto* child = MemNew<Node3D>();
			child->SetName(i == 4 ? "Long_Node_Name_That_Must_Be_Clipped_In_The_Dock" : "Environment" + std::to_string(i));
			scene->AddNode(child);
			if (i == 0)
			{
				auto* hud = MemNew<Control>();
				hud->SetName("HUD");
				hud->SetVisible(false);
				child->AddNode(hud);
			}
		}

		root->SetGuiFocus(nullptr);
		frame();

		std::filesystem::path const captures = project.parent_path() / "Captures";
		std::filesystem::create_directories(captures);
		for (IntVec2 size : { IntVec2(1440, 900), IntVec2(960, 600) })
		{
			root->UpdateResolution(size);
			frame();
			frame();
			SubViewportContainer* view = ui->GetSceneView();
			cameraHeader = headerFor("Camera3D");
			auto* inspectorScroll = FindUIControl<ScrollContainer>(&inspector);
			check(cameraHeader != nullptr && inspectorScroll != nullptr
				&& std::abs(cameraHeader->GetSize().x - (inspectorScroll->GetPageSize().x - theme->GetConstant("content_padding", "Editor"))) < 1.f,
				"class bars use the visible Inspector width even when properties require horizontal scrolling");
			Vector2 const origin = ui->GetViewportOrigin();
			Vector2 const dimensions = ui->GetViewportDimensions();
			IntVec2 const output = view->GetSubViewport()->GetOutputResolution();
			check(output == IntVec2(static_cast<int>(dimensions.x), static_cast<int>(dimensions.y))
				&& view->GetGlobalPosition() == origin, "rulers preserve the scene content resolution contract after resize");
			Vector2 const mapped = ui->ToViewportPos(origin + dimensions * 0.5f);
			check(ui->ToViewportPos(origin) == Vector2::Zero
				&& std::abs(mapped.x - output.x * 0.5f) < 0.01f && std::abs(mapped.y - output.y * 0.5f) < 0.01f,
				"content origin and center map into the scene texture after reserving ruler space");
			Vector2 const edge = ui->ToViewportPos(origin + dimensions - Vector2(1.f, 1.f));
			check(edge.x >= output.x - 2.f && edge.x < output.x && edge.y >= output.y - 2.f && edge.y < output.y,
				"content bottom-right edge maps inside the scene texture");
			auto right = [&](Vector2 point, bool pressed)
			{
				InputEvent event;
				event.m_type = InputEventType::MouseButton;
				event.m_mouseButton = GLFW_MOUSE_BUTTON_RIGHT;
				event.m_pressed = pressed;
				event.m_position = point;
				root->ProcessInputEvent(event);
			};
			right(origin + Vector2(40.f, -10.f), true);
			check(!ui->IsSceneNavigationActive(), "top ruler does not start camera navigation");
			right(origin + Vector2(40.f, -10.f), false);
			right(origin + Vector2(-10.f, 40.f), true);
			check(!ui->IsSceneNavigationActive(), "left ruler does not start camera navigation");
			right(origin + Vector2(-10.f, 40.f), false);
			right(origin + Vector2(40.f, 40.f), true);
			check(ui->IsSceneNavigationActive(), "content area still starts camera navigation");
			right(origin + Vector2(40.f, 40.f), false);
			pointer(InputEventType::MouseButton, origin + Vector2(40.f, -10.f), true);
			pointer(InputEventType::MouseButton, origin + Vector2(40.f, -10.f));
			pointer(InputEventType::MouseButton, origin + Vector2(-10.f, 40.f), true);
			pointer(InputEventType::MouseButton, origin + Vector2(-10.f, 40.f));
			check(editor->GetSelection().GetSelected() == camera->GetObjectID(),
				"ruler clicks leave the scene selection unchanged");
			GPUTexture* texture = engine.m_renderServer->ViewportGetTexture(root->GetViewportRID());
			unsigned const tickX = static_cast<unsigned>(std::round(origin.x + 100.f));
			unsigned const tickY = static_cast<unsigned>(std::round(origin.y - 2.f));
			check(ReadPixel(texture, tickX, tickY) == Color(81, 74, 64)
				&& ReadPixel(texture, tickX + 5, tickY) == Color(29, 28, 26),
				"rendered ruler marks pixel 100 with a sharp line on field background");
			SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
				captures / ("editor-ui-" + std::to_string(size.x) + ".ppm"));
		}

		filePanel.NavigateTo("res://Textures");
		frame();
		Tree* resources = ui->GetFileTreeControl();
		for (int row = 0; row < static_cast<int>(resources->GetInternalMinimumSize().y / resources->GetRowHeight()); ++row)
		{
			Vector2 const point(80.f, (row + 0.5f) * resources->GetRowHeight());
			GuiDragData data;
			if (resources->GetDragData(point, data) && data.m_value == String("res://Textures/preview-triangle.obj"))
			{
				resources->Select(resources->GetItemAtPosition(point), true);
				break;
			}
		}

		ui->GetDockSlot(2)->SetCurrentTab(ui->GetDockSlot(2)->GetPageIndex(&ui->GetPanel<ImportPanel>()));
		root->UpdateResolution(IntVec2(1440, 900));
		frame();
		frame();
		check(FindUIControl<SpinBox>(&ui->GetPanel<ImportPanel>()) != nullptr, "Import displays real options for its own preview fixture");
		SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), captures / "editor-ui-import.ppm");
		ui->GetDockSlot(2)->SetCurrentTab(ui->GetDockSlot(2)->GetPageIndex(&inspector));
		filePanel.NavigateTo("res://");
		frame();
		click(static_cast<Control*>(productionBar->GetChildren()[0]));
		frame();
		SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), captures / "editor-ui-menu.ppm");
		key(GLFW_KEY_ESCAPE);
		root->SetGuiFocus(nullptr);
		frame();

		Ref<Theme> editorTheme = editor->GetEditorTheme();
		editorTheme->SetFontSize("ruler_font_size", "ViewportPanel", 15);
		editorTheme->SetColor("ruler_major_color", "ViewportPanel", Color(130, 110, 80));
		frame();
		Vector2 const themedOrigin = ui->GetViewportOrigin();
		check(ReadPixel(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
			static_cast<unsigned>(std::round(themedOrigin.x + 100.f)),
			static_cast<unsigned>(std::round(themedOrigin.y - 2.f))) == Color(130, 110, 80),
			"ruler Theme edits redraw without changing scene coordinate mapping");
		editorTheme->SetFontSize("ruler_font_size", "ViewportPanel", 13);
		editorTheme->SetColor("ruler_major_color", "ViewportPanel", Color(81, 74, 64));
		frame();

		if (preview)
		{
			auto* overlay = MemNew<Control>();
			overlay->SetMouseFilter(Control::MouseFilter::Ignore);
			overlay->SetClipContents(true);
			ui->GetSceneView()->AddNode(overlay);
			std::vector<ColorRect*> gridLines;
			for (int i = 0; i < 96; ++i)
			{
				auto* line = MemNew<ColorRect>();
				line->SetColor(Color(41, 41, 41));
				overlay->AddNode(line);
				gridLines.push_back(line);
			}

			auto* axisX = MemNew<ColorRect>();
			axisX->SetColor(Color(121, 81, 81));
			overlay->AddNode(axisX);
			auto* axisY = MemNew<ColorRect>();
			axisY->SetColor(Color(82, 112, 98));
			overlay->AddNode(axisY);
			auto* readout = MemNew<Label>();
			readout->SetText("X -  Y -                         Grid 24 px");
			readout->SetThemeTypeVariation("EditorStatusLabel");
			overlay->AddNode(readout);
			bool grid = true;
			bool running = false;
			bool paused = false;
			auto* play = static_cast<Button*>(FindNamedUI(ui->GetRootControl(), "Play"));
			auto* pause = static_cast<Button*>(FindNamedUI(ui->GetRootControl(), "Pause"));
			auto* stop = static_cast<Button*>(FindNamedUI(ui->GetRootControl(), "Stop"));
			auto updateRun = [&]()
			{
				play->SetPressedNoSignal(running && !paused);
				pause->SetPressedNoSignal(paused);
				pause->SetDisabled(!running);
				stop->SetDisabled(!running);
			};
			play->SetToggleMode(true);
			pause->SetToggleMode(true);
			play->SetPressedCallback([&]() { running = true; paused = false; updateRun(); });
			pause->SetPressedCallback([&]() { paused = !paused; updateRun(); });
			stop->SetPressedCallback([&]() { running = false; paused = false; updateRun(); });
			std::vector<Button*> tools;
			for (char const* name : { "ToolSelect", "ToolMove", "ToolRotate", "ToolScale" })
			{
				auto* button = static_cast<Button*>(FindNamedUI(&ui->GetPanel<ViewportPanel>(), name));
				button->SetDisabled(false);
				button->SetToggleMode(true);
				tools.push_back(button);
				button->SetPressedCallback([&, button]()
				{
					for (Button* tool : tools)
					{
						tool->SetPressedNoSignal(tool == button);
					}
				});
			}

			tools.front()->SetPressedNoSignal(true);
			auto* gridButton = static_cast<Button*>(FindNamedUI(&ui->GetPanel<ViewportPanel>(), "Grid"));
			gridButton->SetDisabled(false);
			gridButton->SetToggleMode(true);
			gridButton->SetPressedNoSignal(true);
			gridButton->SetPressedCallback([&]() { grid = !grid; });
			auto* snap = static_cast<Button*>(FindNamedUI(&ui->GetPanel<ViewportPanel>(), "Snap"));
			snap->SetDisabled(false);
			snap->SetToggleMode(true);
			int zoom = 100;
			auto* zoomLabel = static_cast<Label*>(FindNamedUI(&ui->GetPanel<ViewportPanel>(), "ZoomReadout"));
			for (char const* name : { "ZoomOut", "ZoomIn", "CenterView" })
			{
				auto* button = static_cast<Button*>(FindNamedUI(&ui->GetPanel<ViewportPanel>(), name));
				button->SetDisabled(false);
				button->SetPressedCallback([&, name]()
				{
					zoom = std::string(name) == "CenterView" ? 100 : std::clamp(zoom + (std::string(name) == "ZoomIn" ? 25 : -25), 25, 400);
					zoomLabel->SetText(std::to_string(zoom) + "%");
				});
			}

			PropertyInfo demoInfo(Variant::Type::Float, "z_index", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Inspector);
			InspectorProperty* demoProperty = nullptr;
			auto createModifiedSample = [&]()
			{
				delete demoProperty;
				demoProperty = InspectorProperty::Create(demoInfo, "Preview::Modified", [&](Variant const& value) { demoProperty->UpdateValue(value); });
				inspector.GetContentControl()->AddNode(demoProperty->GetControl());
				demoProperty->SetRevertValue(Variant(0.f));
				demoProperty->UpdateValue(Variant(2.f));
			};
			createModifiedSample();
			auto* window = engine.m_windowSystem->GetGLFWWindow();
			glfwSetWindowTitle(window, "Ming Editor Style Preview - F1 1440x900 / F2 960x600");
			glfwRestoreWindow(window);
			glfwSetWindowSize(window, 1440, 900);
			while (!glfwWindowShouldClose(window))
			{
				if (glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS)
				{
					glfwSetWindowSize(window, 1440, 900);
				}
				else if (glfwGetKey(window, GLFW_KEY_F2) == GLFW_PRESS)
				{
					glfwSetWindowSize(window, 960, 600);
				}

				int width = 0;
				int height = 0;
				glfwGetFramebufferSize(window, &width, &height);
				root->UpdateResolution(IntVec2(width, height));
				Vector2 const area = ui->GetSceneView()->GetSize();
				overlay->UpdateSize(area);
				for (int i = 0; i < 96; ++i)
				{
					gridLines[i]->SetVisible(grid);
					bool const vertical = i < 48;
					float const at = static_cast<float>(i % 48) * 30.f * zoom / 100.f;
					gridLines[i]->SetPosition(vertical ? Vector2(at, 0.f) : Vector2(0.f, at));
					gridLines[i]->UpdateSize(vertical ? Vector2(1.f, area.y) : Vector2(area.x, 1.f));
				}

				axisX->SetPosition(Vector2(0.f, area.y * 0.35f));
				axisX->UpdateSize(Vector2(area.x, 1.f));
				axisY->SetPosition(Vector2(area.x * 0.25f, 0.f));
				axisY->UpdateSize(Vector2(1.f, area.y));
				readout->SetPosition(Vector2(10.f, std::max(0.f, area.y - 25.f)));
				frame();
				if (demoProperty->GetControl() == nullptr && editor->GetSelection().GetSelected().IsValid())
				{
					createModifiedSample();
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(16));
			}

			delete demoProperty;
		}
	}

	ThemeDatabase::Shutdown();
	ResourceLoader::Shutdown();
	EditorIcons::Shutdown();
	engine.Shutdown();
	ClassDatabase::Shutdown();
	std::printf("EDITOR_UI checks=%d failures=%d\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
