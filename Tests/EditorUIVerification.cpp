#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/Gizmos/GizmoComponent.hpp"
#include "MingEngine/Editor/Gizmos/TransformGizmo3D.hpp"
#include "MingEngine/Scene/3D/MeshInstance3D.hpp"
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
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "ColorPickerVerification.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyFloat.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "RenderReadback.hpp"
#include "UILayoutPreview.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Core/Render/SurfaceTool.hpp"

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
	if (auto* control = dynamic_cast<Control*>(node);
		control != nullptr && node->GetName() == name && !node->IsQueuedForDeletion())
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
	bool  preview = false;
	float uiScale = 1.25f;
	for (int i = 2; i < argc; ++i)
	{
		std::string const argument(argv[i]);
		if (argument == "--layout-preview")
		{
			preview = true;
		}
		else if (argument == "--preview")
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

	bool const layoutPreview = argc >= 3 && std::string(argv[2]) == "--layout-preview";

	int  checks   = 0;
	int  failures = 0;
	auto check    = [&](bool result, char const* message)
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
	config.m_fileSystemConfig.m_resourceRoot       = project;
	config.m_scriptSystemConfig.m_isEnabled        = false;
	config.m_audioConfig.m_isEnable                = false;
	config.m_networkConfig.m_isEnable              = false;
	config.m_windowConfig.m_appName                = "Editor UI Verification";
	config.m_windowConfig.m_isResizable            = true;
	config.m_windowConfig.m_isBorderlessFullscreen = false;
	Engine engine(config);
	ClassDatabase::Startup();
	RegisterAllTypes();
	engine.Startup();
	std::ofstream(project / L"project.ming") << R"({"type":"ProjectSettings","version":1,"start_scene_path":""})";
	std::string settingsError;
	check(ProjectSettings::Load(project, settingsError), "fixture installs its own project settings");
	// 1) Keep scripted input independent of desktop focus, mouse motion and resize messages.
	auto* scriptedWindow      = engine.m_windowSystem->GetGLFWWindow();
	auto  keyCallback         = glfwSetKeyCallback(scriptedWindow, nullptr);
	auto  mouseButtonCallback = glfwSetMouseButtonCallback(scriptedWindow, nullptr);
	auto  cursorPosCallback   = glfwSetCursorPosCallback(scriptedWindow, nullptr);
	auto  characterCallback   = glfwSetCharCallback(scriptedWindow, nullptr);
	auto  scrollCallback      = glfwSetScrollCallback(scriptedWindow, nullptr);
	auto  windowFocusCallback = glfwSetWindowFocusCallback(scriptedWindow, nullptr);
	auto  cursorEnterCallback = glfwSetCursorEnterCallback(scriptedWindow, nullptr);
	auto  framebufferCallback = glfwSetFramebufferSizeCallback(scriptedWindow, nullptr);

	if (layoutPreview)
	{
		SceneTree previewTree;
		Label*    sizeLabel = BuildUILayoutPreview(&previewTree);
		auto*     root      = static_cast<Viewport*>(previewTree.GetRoot());
		auto*     window    = engine.m_windowSystem->GetGLFWWindow();
		glfwRestoreWindow(window);
		glfwSetWindowSize(window, 1440, 900);
		glfwSetKeyCallback(window, keyCallback);
		glfwSetMouseButtonCallback(window, mouseButtonCallback);
		glfwSetCursorPosCallback(window, cursorPosCallback);
		glfwSetCharCallback(window, characterCallback);
		glfwSetScrollCallback(window, scrollCallback);
		glfwSetWindowFocusCallback(window, windowFocusCallback);
		glfwSetCursorEnterCallback(window, cursorEnterCallback);
		glfwSetFramebufferSizeCallback(window, framebufferCallback);
		while (!glfwWindowShouldClose(window))
		{
			engine.BeginFrame();
			IntVec2 const resolution = engine.m_windowSystem->GetClientDimensions();
			root->UpdateResolution(resolution);
			sizeLabel->SetText(Stringf("Viewport: %d x %d", resolution.x, resolution.y));
			previewTree.UpdateScene(1.f / 60.f);
			engine.m_renderServer->Render();
			previewTree.FlushPendingNode();
			engine.EndFrame();
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}
	else
	{
		SceneTree tree;
		auto*     root = static_cast<Viewport*>(tree.GetRoot());
		root->UpdateResolution(IntVec2(1440, 900));
		auto* scope = MemNew<Control>();
		root->AddNode(scope);
		scope->UpdateSize(Vector2(1440.f, 900.f));
		auto theme = EditorThemeManager::CreateScaledTheme(EditorThemeManager::CreateTheme(), uiScale);
		ThemeDatabase::SetThemeContext(scope, { theme });
		auto pointer = [&](InputEventType type, Vector2 position, bool pressed = false)
		{
			InputEvent event;
			event.m_type        = type;
			event.m_position    = position;
			event.m_pressed     = pressed;
			event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
			root->ProcessInputEvent(event);
		};
		auto key = [&](int code)
		{
			InputEvent event;
			event.m_type    = InputEventType::Key;
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
		bar->AddMenu(
			"First",
			[&]()
			{
				return std::vector<MenuBar::MenuItem>{ { "Disabled", {} },
													   { "", {}, "", true },
													   { "Action",
														 [&]()
														 {
															 ++actions;
														 },
														 "Ctrl+A",
														 false,
														 true,
														 true } };
			});
		bar->AddMenu(
			"Second",
			[]()
			{
				return std::vector<MenuBar::MenuItem>{ { "Unavailable", {} } };
			});
		bar->UpdateSize(bar->GetCombinedMinimumSize());
		tree.FlushContainerUpdate();
		auto* first  = static_cast<Button*>(bar->GetChildren()[0]);
		auto* second = static_cast<Button*>(bar->GetChildren()[1]);
		check(
			first->GetFontSize() == theme->GetDefaultFontSize()
				&& bar->GetSize().y >= theme->GetConstant("minimum_height", "MenuBar")
				&& first->GetSize().y == bar->GetSize().y,
			"editor menu fonts and title hit areas use configured menu height");
		click(first);
		check(
			bar->GetActiveMenu() == 0 && root->GetGuiPopup() != nullptr && first->IsPressed(),
			"click opens menu and marks its title active");
		auto* focused = dynamic_cast<Button*>(root->GetGuiFocus());
		check(
			focused != nullptr && focused->GetText() == String("Action"),
			"menu initially skips disabled items and separators");
		check(
			focused != nullptr && FindNamedUI(focused, "Shortcut") != nullptr
				&& FindNamedUI(focused, "Check") != nullptr,
			"menu row has independent shortcut and check columns");
		pointer(InputEventType::MouseMotion, second->GetGlobalPosition() + second->GetSize() * 0.5f);
		check(
			bar->GetActiveMenu() == 1 && second->IsPressed() && !first->IsPressed(),
			"hover switches an open menu through its popup overlay");
		key(GLFW_KEY_RIGHT);
		check(bar->GetActiveMenu() == 0, "Right wraps to the first menu");
		key(GLFW_KEY_LEFT);
		check(bar->GetActiveMenu() == 1, "Left wraps to the last menu");
		key(GLFW_KEY_ESCAPE);
		check(
			root->GetGuiPopup() == nullptr && bar->GetActiveMenu() == -1 && bar->HasFocus() && !second->IsPressed(),
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
		menuFocus.m_type    = InputEventType::WindowFocus;
		menuFocus.m_focused = false;
		root->ProcessInputEvent(menuFocus);
		check(
			root->GetGuiPopup() == nullptr && bar->GetActiveMenu() == -1 && !first->IsPressed(),
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
		auto dividerPosition = [&]()
		{
			return split->GetGlobalPosition() + split->GetDividerRect().GetCenter();
		};
		pointer(InputEventType::MouseMotion, dividerPosition());
		check(
			engine.m_windowSystem->GetCursorShape() == CursorShape::ResizeHorizontal,
			"horizontal split hover selects the system horizontal resize cursor");
		pointer(InputEventType::MouseButton, dividerPosition(), true);
		pointer(InputEventType::MouseMotion, Vector2(10.f, 100.f));
		check(
			engine.m_windowSystem->GetCursorShape() == CursorShape::ResizeHorizontal,
			"captured split keeps resize cursor when pointer leaves its divider");
		pointer(InputEventType::MouseButton, Vector2(10.f, 100.f));
		check(engine.m_windowSystem->GetCursorShape() == CursorShape::Arrow, "release outside split restores arrow");
		split->SetVertical(true);
		tree.FlushContainerUpdate();
		pointer(InputEventType::MouseMotion, dividerPosition());
		check(
			engine.m_windowSystem->GetCursorShape() == CursorShape::ResizeVertical,
			"vertical split hover selects the system vertical resize cursor");
		InputEvent focus;
		focus.m_type    = InputEventType::WindowFocus;
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
		actionTree->SetItemActionCallback(
			[&](uint64_t, int)
			{
				++tailClicks;
			});
		Tree::Item item{ 1, 0, "LongNameThatMustLeaveRoomForTailIcons", true, true };
		item.m_actions.push_back({ EditorIcons::GetIconTexture("Visible"), Color::White, true });
		actionTree->SetItems({ item });
		actionTree->UpdateSize(Vector2(220.f, 35.f));
		tree.FlushContainerUpdate();
		Vector2 const tail = actionTree->GetGlobalPosition() + actionTree->GetItemActionRect(1, 0).GetCenter();
		pointer(InputEventType::MouseButton, tail, true);
		pointer(InputEventType::MouseButton, tail);
		check(
			tailClicks == 1 && actionTree->GetSelected() == 0,
			"tree tail action does not select or activate the row");
		GuiDragData tailDrag;
		check(
			!actionTree->GetDragData(actionTree->GetItemActionRect(1, 0).GetCenter(), tailDrag),
			"tree tail action does not initiate row dragging");
		actionTree->Select(1);
		actionTree->EditSelected();
		check(
			FindUIControl<LineEdit>(actionTree)->GetLocalPosition().x + FindUIControl<LineEdit>(actionTree)->GetSize().x
				<= actionTree->GetItemActionRect(1, 0).m_mins.x,
			"rename field reserves the same tail area as drawing and hit testing");
		scope->QueueFree();
		tree.FlushPendingNode();

		auto* editor = MemNew<EditorNode>(uiScale);
		root->AddNode(editor);
		EditorUI* ui    = editor->m_editorUI;
		auto      frame = [&]()
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
		check(
			productionBar != nullptr
				&& ui->GetDockSplit(0)->GetGlobalPosition().y
					   == editor->GetEditorTheme()->GetConstant("top_bar_height", "Editor"),
			"production editor uses MenuBar and the configured top bar height");
		check(
			editor->GetEditorTheme()->GetDefaultFontSize() == theme->GetDefaultFontSize()
				&& ThemeDatabase::GetDefaultTheme()->GetDefaultFontSize() == 16,
			"scaled editor dimensions leave runtime theme unchanged");
		auto const contextThemes = ThemeDatabase::GetThemeContext(ui->GetRootControl());
		check(
			contextThemes.size() == 1 && contextThemes.front() == editor->GetEditorTheme(),
			"editor context contains only the generated Theme");
		check(
			ThemeDatabase::GetThemeContext(ui->GetSceneView()->GetSubViewport()).empty(),
			"scene preview remains isolated from editor scaling");
		check(
			ui->GetDockSlot(3)->GetTabBarRect().m_mins.y >= ui->GetDockSlot(3)->GetPageRect().m_maxs.y,
			"Output tabs and their input geometry sit below its content");
		if (productionBar != nullptr)
		{
			auto menuPixels = [&]()
			{
				std::filesystem::path const path = project.parent_path() / "Captures" / "menu-stability.ppm";
				std::filesystem::create_directories(path.parent_path());
				SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), path);
				std::ifstream input(path, std::ios::binary);
				std::string   magic;
				int           width   = 0;
				int           height  = 0;
				int           maximum = 0;
				input >> magic >> width >> height >> maximum;
				input.get();
				std::vector<char> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
				std::vector<char> pixels;
				Vector2 const     start = productionBar->GetGlobalPosition();
				Vector2 const     end   = start + productionBar->GetSize();
				for (int y = std::max(0, static_cast<int>(start.y));
					 y < std::min(height, static_cast<int>(std::ceil(end.y)));
					 ++y)
				{
					int const left  = std::clamp(static_cast<int>(start.x), 0, width);
					int const right = std::clamp(static_cast<int>(std::ceil(end.x)), left, width);
					pixels.insert(
						pixels.end(),
						bytes.begin() + (y * width + left) * 3,
						bytes.begin() + (y * width + right) * 3);
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

			check(
				!stablePixels.empty() && stablePixels == menuPixels(),
				"menu pixels return unchanged after repeated open, hover switching, close and redraw");
		}
		Tree* files = ui->GetFileTreeControl();
		files->GrabFocus();
		check(
			files->HasFocus() && files->GetThemeStyleBox("focus")->GetBorderWidth(ThemeStyleBox::Side::Right) == 0.f,
			"editor tree keeps keyboard focus without drawing its focus border");
		if (productionBar != nullptr)
		{
			click(static_cast<Control*>(productionBar->GetChildren()[1]));
			check(
				root->GetGuiPopup() != nullptr && productionBar->GetActiveMenu() == 1,
				"production Project menu uses the independent MenuBar");
			key(GLFW_KEY_ESCAPE);
		}

		auto white = CreateRef<Image>();
		white->LoadFromRGBA8(IntVec2(2, 2), std::vector<uint8_t>(16, 255));
		auto texture     = CreateRef<TextureResource>();
		texture->m_image = white;
		texture->InitGPUResources();
		auto* tintProbe = MemNew<TextureRect>();
		root->AddNode(tintProbe);
		tintProbe->SetTexture(texture);
		tintProbe->SetTint(Color(147, 208, 221));
		tintProbe->SetPosition(Vector2(1400.f, 5.f));
		tintProbe->UpdateSize(Vector2(20.f, 20.f));
		frame();
		check(
			ReadPixel(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), 1405, 10)
				== Color(147, 208, 221),
			"TextureRect tint reaches the actual rendered texture");
		tintProbe->SetStretchMode(TextureRect::StretchMode::Tile);
		tintProbe->SetHorizontalFlip(true);
		frame();
		check(
			ReadPixel(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), 1405, 10)
				== Color(147, 208, 221),
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
		auto            headerFor = [&](char const* name) -> Control*
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
		Control* nodeHeader   = headerFor("Node3D");
		check(
			cameraHeader != nullptr && nodeHeader != nullptr && cameraHeader->IsVisibleInHierarchy()
				&& nodeHeader->IsVisibleInHierarchy()
				&& cameraHeader->GetGlobalPosition().y < nodeHeader->GetGlobalPosition().y,
			"Inspector shows class headers from derived class to base class");
		if (cameraHeader != nullptr)
		{
			auto* icon  = FindUIControl<TextureRect>(cameraHeader);
			auto* title = FindUIControl<Label>(cameraHeader);
			check(
				icon != nullptr && icon->GetTint() == EditorIcons::GetIconColor("Camera3D", &inspector),
				"Inspector class icon uses the same category color as Scene");
			Color const changedTint(120, 90, 180);
			editor->GetEditorTheme()->SetColor("t3d", "EditorIcons", changedTint);
			frame();
			check(icon != nullptr && icon->GetTint() == changedTint, "Inspector icon tint follows live Theme edits");
			editor->GetEditorTheme()->SetColor("t3d", "EditorIcons", Color(166, 139, 212));
			frame();
			check(
				icon != nullptr && icon->GetTexture().IsValid() && title != nullptr
					&& std::abs(
						   (icon->GetGlobalPosition().x + title->GetGlobalPosition().x + title->GetSize().x) * 0.5f
						   - (cameraHeader->GetGlobalPosition().x + cameraHeader->GetSize().x * 0.5f))
						   < 1.f,
				"class icon and label form one centered group");
			title->SetText("VeryLongClassNameThatMustNotExpandTheInspectorPanel");
			frame();
			frame();
			check(
				title->GetGlobalPosition().x + title->GetSize().x
						<= cameraHeader->GetGlobalPosition().x + cameraHeader->GetSize().x
					&& icon->GetSize().x == theme->GetConstant("icon_size", "Button"),
				"long class names clip inside the bar without shrinking its icon");
			title->SetText("Camera3D");
			frame();
		}

		Button* transformGroup = dynamic_cast<Button*>(FindNamedUI(&inspector, "InspectorGroupHeading_Transform"));
		Control* positionField = FindNamedUI(&inspector, "position");
		Control* rotationField = FindNamedUI(&inspector, "rotation");
		if (transformGroup != nullptr && positionField != nullptr && rotationField != nullptr)
		{
			auto* title = FindUIControl<Label>(positionField);
			auto style = transformGroup->GetThemeStyleBox("normal");
			float const textStart = transformGroup->GetGlobalPosition().x
				+ style->GetContentMargin(ThemeStyleBox::Side::Left)
				+ transformGroup->GetThemeConstant("icon_size", "EditorGroupButton")
				+ transformGroup->GetThemeConstant("icon_separation", "EditorGroupButton");
			check(title != nullptr && std::abs(title->GetGlobalPosition().x - textStart) < 0.01f,
				"group contents align with the heading text");
			auto* x = FindNamedUI(positionField, "x");
			auto* y = FindNamedUI(positionField, "y");
			auto* z = FindNamedUI(positionField, "z");
			check(x != nullptr && y != nullptr && z != nullptr
				&& std::abs(x->GetSize().x - y->GetSize().x) < 0.01f
				&& std::abs(y->GetSize().x - z->GetSize().x) < 0.01f
				&& x->GetGlobalPosition().y >= title->GetGlobalPosition().y + title->GetSize().y,
				"Vector3 uses equal-width components below its title");
			if (x != nullptr)
			{
				check(FindUIControl<Label>(x)->GetFontColor() == theme->GetColor("property_color_x", "Editor"),
					"vector axis labels use the theme axis color");
			}

			auto* rotationX = FindNamedUI(rotationField, "x");
			check(FindUIControl<Label>(rotationField)->GetText() == String("Rotation")
				&& rotationX != nullptr && FindUIControl<SpinBox>(rotationX)->GetLineEdit()->GetText() == String("0"),
				"rotation keeps units outside the editable numeric text");
			bool degree = false;
			if (rotationX != nullptr)
			{
				for (Node* child : FindUIControl<SpinBox>(rotationX)->GetParent()->GetChildren())
				{
					if (auto* label = dynamic_cast<Label*>(child))
					{
						degree = degree || label->GetText() == String(U"\u00b0");
					}
				}
			}

			check(degree, "Euler component displays a separate degree suffix");
		}

		auto* loadScript = FindNamedUI(&inspector, "LoadScript");
		auto* scriptMenu = dynamic_cast<Button*>(FindNamedUI(&inspector, "ScriptMenu"));
		check(loadScript != nullptr && scriptMenu != nullptr
			&& loadScript->GetCombinedMinimumSize().x == theme->GetConstant("icon_size", "EditorScriptAction")
				+ loadScript->GetThemeStyleBox("normal")->GetMinimumSize().x
			&& scriptMenu->GetCombinedMinimumSize().x == theme->GetConstant("icon_size", "EditorScriptMenu")
				+ scriptMenu->GetThemeStyleBox("normal")->GetMinimumSize().x
			&& scriptMenu->GetIcon() == theme->GetIcon("Dropdown", "EditorIcons"),
			"Script actions use compact icon slots and the dedicated triangle");

		check(transformGroup != nullptr, "Node3D properties are grouped under Transform");
		if (transformGroup != nullptr)
		{
			click(transformGroup);
			frame();
			check(positionField != nullptr && !positionField->IsVisibleInHierarchy(), "fold hides section contents");
			LineEdit* search = FindUIControl<LineEdit>(&inspector);
			search->SetText("Transform");
			frame();
			check(
				transformGroup->IsVisibleInHierarchy() && nodeHeader->IsVisibleInHierarchy()
					&& positionField != nullptr && positionField->IsVisibleInHierarchy(),
				"group-name search expands matching group");
			search->SetText("");
			frame();
			check(!positionField->IsVisibleInHierarchy(), "clearing search restores the previous folded state");
			click(transformGroup);
			frame();
		}

		auto* filter = FindUIControl<LineEdit>(&inspector);
		filter->SetText("position");
		frame();
		check(
			cameraHeader != nullptr && nodeHeader != nullptr && !cameraHeader->IsVisibleInHierarchy()
				&& nodeHeader->IsVisibleInHierarchy(),
			"property search hides the complete unrelated class header");
		SpinBox* position = positionField != nullptr ? FindUIControl<SpinBox>(positionField) : nullptr;
		check(rotationField != nullptr && !rotationField->IsVisibleInHierarchy(), "property-name search hides unmatched siblings");

		if (position != nullptr)
		{
			position->Step(1);
			check(
				camera->GetLocalPosition().x > 0.f && editor->IsSceneDirty(),
				"inherited Inspector property still writes through its setter");
		}

		filter->SetText("no matching property");
		frame();
		check(
			cameraHeader != nullptr && nodeHeader != nullptr && !cameraHeader->IsVisibleInHierarchy()
				&& !nodeHeader->IsVisibleInHierarchy(),
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
		SubViewportContainer* sceneView        = ui->GetSceneView();
		Vector2 const         beforeDockResize = sceneView->GetSize();
		ui->GetDockSplit(1)->SetSplitRatio(ui->GetDockSplit(1)->GetSplitRatio() - 0.05f);
		frame();
		frame();
		check(
			sceneView->GetSize().x <= beforeDockResize.x
				&& sceneView->GetSubViewport()->GetOutputResolution().x == static_cast<int>(sceneView->GetSize().x),
			"dock resize respects minimum widths and keeps the content texture aligned with the viewport content bounds");
		ui->ResetNativeLayout();
		frame();
		frame();


		check(SpinBox::FormatValue(Variant(0.1f)) == "0.1", "float fields use shortest round-trip representation");
		for (int i = 0; i < 4; ++i)
		{
			TabContainer* slot = ui->GetDockSlot(i);
			check(
				slot->GetTabInsertIndex(slot->GetTitleActions()->GetLocalPosition() + Vector2(5.f, 5.f)) == -1,
				"dock action area cannot become a tab insertion target");
		}

		OutputPanel& outputPanel = ui->GetPanel<OutputPanel>();
		outputPanel.AddMessage({ "10:24:01", OutputPanel::Level::Info, "Project loaded: StyleVerification" });
		outputPanel.AddMessage({ "10:24:02", OutputPanel::Level::Warning, "Texture uses default import settings" });
		outputPanel.AddMessage({ "10:24:03", OutputPanel::Level::Error, "Preview diagnostic message" });
		frame();
		click(FindNamedUI(&outputPanel, "StatusError"));
		frame();
		check(outputPanel.GetMessageCount() == 3, "level filtering leaves source messages intact");
		outputPanel.ClearMessages();
		frame();
		check(outputPanel.GetMessageCount() == 0, "Clear removes Output messages");
		click(FindNamedUI(&outputPanel, "StatusError"));
		ui->ToggleOutputCollapsed();
		frame();
		check(
			outputPanel.IsCollapsed()
				&& ui->GetDockSlot(3)->GetSize().y <= ui->GetDockSlot(3)->GetTabBarRect().GetHeight() + 1.f,
			"Output collapse reserves only its header");
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
		check(
			filePanel.GetDirectory() == VirtualPath("res://Scenes"),
			"path navigation changes the displayed directory");
		filePanel.NavigateTo("res://MissingDirectory");
		check(
			filePanel.GetDirectory() == VirtualPath("res://Scenes"),
			"invalid directory leaves browsing state intact");
		filePanel.NavigateTo("res://");
		frame();
		click(FindNamedUI(&filePanel, "Back"));
		frame();
		check(filePanel.GetDirectory() == VirtualPath("res://Scenes"), "Back restores the previous directory");
		click(FindNamedUI(&filePanel, "Forward"));
		frame();
		check(filePanel.GetDirectory() == VirtualPath("res://"), "Forward restores the next directory");
		outputPanel.AddMessage({ "10:24:01", OutputPanel::Level::Info, "Project loaded: StyleVerification" });
		outputPanel.AddMessage(
			{ "10:24:02", OutputPanel::Level::Warning, "Preview resource has default import settings" });
		outputPanel.AddMessage({ "10:24:03", OutputPanel::Level::Error, "Preview diagnostic message" });
		for (int i = 0; i < 5; ++i)
		{
			auto* child = MemNew<Node3D>();
			child->SetName(
				i == 4 ? "Long_Node_Name_That_Must_Be_Clipped_In_The_Dock" : "Environment" + std::to_string(i));
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
			cameraHeader               = headerFor("Camera3D");
			auto* inspectorScroll      = FindUIControl<ScrollContainer>(&inspector);
			check(
				cameraHeader != nullptr && inspectorScroll != nullptr
					&& std::abs(
						   cameraHeader->GetSize().x
						   - (inspectorScroll->GetPageSize().x - theme->GetConstant("content_padding", "Editor")))
						   < 1.f,
				"class bars use the visible Inspector width even when properties require horizontal scrolling");
			Vector2 const origin     = ui->GetViewportOrigin();
			Vector2 const dimensions = ui->GetViewportDimensions();
			IntVec2 const output     = view->GetSubViewport()->GetOutputResolution();
			check(
				output == IntVec2(static_cast<int>(dimensions.x), static_cast<int>(dimensions.y))
					&& view->GetGlobalPosition() == origin,
				"viewport preserves its content resolution contract after resize");
			Vector2 const mapped = ui->ToViewportPos(origin + dimensions * 0.5f);
			check(
				ui->ToViewportPos(origin) == Vector2::Zero && std::abs(mapped.x - output.x * 0.5f) < 0.01f
					&& std::abs(mapped.y - output.y * 0.5f) < 0.01f,
				"content origin and center map into the scene texture without pixel ruler margins");
			Vector2 const edge = ui->ToViewportPos(origin + dimensions - Vector2(1.f, 1.f));
			check(
				edge.x >= output.x - 2.f && edge.x < output.x && edge.y >= output.y - 2.f && edge.y < output.y,
				"content bottom-right edge maps inside the scene texture");
			auto right = [&](Vector2 point, bool pressed)
			{
				InputEvent event;
				event.m_type        = InputEventType::MouseButton;
				event.m_mouseButton = GLFW_MOUSE_BUTTON_RIGHT;
				event.m_pressed     = pressed;
				event.m_position    = point;
				root->ProcessInputEvent(event);
			};
			right(origin + Vector2(40.f, -10.f), true);
			check(!ui->IsSceneNavigationActive(), "viewport toolbar does not start camera navigation");
			right(origin + Vector2(40.f, -10.f), false);
			right(origin + Vector2(-10.f, 40.f), true);
			check(!ui->IsSceneNavigationActive(), "neighboring dock does not start camera navigation");
			right(origin + Vector2(-10.f, 40.f), false);
			right(origin + Vector2(40.f, 40.f), true);
			check(ui->IsSceneNavigationActive(), "content area still starts camera navigation");
			right(origin + Vector2(40.f, 40.f), false);
			pointer(InputEventType::MouseButton, origin + Vector2(40.f, -10.f), true);
			pointer(InputEventType::MouseButton, origin + Vector2(40.f, -10.f));
			pointer(InputEventType::MouseButton, origin + Vector2(-10.f, 40.f), true);
			pointer(InputEventType::MouseButton, origin + Vector2(-10.f, 40.f));
			check(
				editor->GetSelection().GetSelected() == camera->GetObjectID(),
				"toolbar and neighboring dock clicks preserve the scene selection");
			SaveRenderedFrame(
				engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
				captures / ("editor-ui-" + std::to_string(size.x) + ".ppm"));
		}

		filePanel.NavigateTo("res://Textures");
		frame();
		Tree* resources = ui->GetFileTreeControl();
		for (int row = 0; row < static_cast<int>(resources->GetInternalMinimumSize().y / resources->GetRowHeight());
			 ++row)
		{
			Vector2 const point(80.f, (row + 0.5f) * resources->GetRowHeight());
			GuiDragData   data;
			if (resources->GetDragData(point, data) && data.m_value == String("res://Textures/preview-triangle.obj"))
			{
				resources->Select(resources->GetItemAtPosition(point), true);
				break;
			}
		}

		ui->GetDockSlot(0)->SetCurrentTab(ui->GetDockSlot(0)->GetPageIndex(&ui->GetPanel<ImportPanel>()));
		root->UpdateResolution(IntVec2(1440, 900));
		frame();
		frame();
		check(
			FindUIControl<SpinBox>(&ui->GetPanel<ImportPanel>()) != nullptr,
			"Import displays real options for its own preview fixture");
		SaveRenderedFrame(
			engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
			captures / "editor-ui-import.ppm");
		ui->GetDockSlot(2)->SetCurrentTab(ui->GetDockSlot(2)->GetPageIndex(&inspector));
		filePanel.NavigateTo("res://");
		frame();
		click(static_cast<Control*>(productionBar->GetChildren()[0]));
		frame();
		SaveRenderedFrame(
			engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
			captures / "editor-ui-menu.ppm");
		key(GLFW_KEY_ESCAPE);
		root->SetGuiFocus(nullptr);
		frame();

		// 1) Exercise creation through the production modal and keyboard routes.
		auto textInput = [&](LineEdit* edit, std::string const& value)
		{
			edit->GrabFocus();
			edit->SelectAll();
			for (unsigned char ch : value)
			{
				InputEvent event;
				event.m_type      = InputEventType::Text;
				event.m_codepoint = ch;
				root->ProcessInputEvent(event);
			}
			frame();
		};
		std::function<bool(Node*, String const&)> hasLabel = [&](Node* node, String const& text)
		{
			if (auto* label = dynamic_cast<Label*>(node); label != nullptr && label->IsVisibleInHierarchy()
														  && !node->IsQueuedForDeletion() && label->GetText() == text)
			{
				return true;
			}

			for (Node* child : node->GetChildren())
			{
				if (hasLabel(child, text))
				{
					return true;
				}
			}
			return false;
		};
		auto* outputSearch = static_cast<LineEdit*>(FindNamedUI(&outputPanel, "SearchOutput"));
		textInput(outputSearch, "DIAGNOSTIC");
		click(FindNamedUI(&outputPanel, "Info"));
		click(FindNamedUI(&outputPanel, "StatusWarning"));
		frame();
		check(
			hasLabel(&outputPanel, "Preview diagnostic message")
				&& !hasLabel(&outputPanel, "Preview resource has default import settings"),
			"case-insensitive Output search intersects the enabled levels");
		click(FindNamedUI(&outputPanel, "ClearOutputSearch"));
		check(
			outputSearch->GetText().IsEmpty() && outputPanel.GetMessageCount() == 3,
			"clearing search keeps the source log intact");
		click(FindNamedUI(&outputPanel, "Info"));
		click(FindNamedUI(&outputPanel, "StatusWarning"));
		engine.m_fileSystem->WriteText("res://.ming/Editor/CreateNodeState.json", "{\"favorites\":[],\"recent\":[]}");
		auto* creationParent = MemNew<Node3D>();
		creationParent->SetName("CreationFixture");
		scene->AddNode(creationParent);
		editor->GetSelection().SetSelected(creationParent->GetObjectID());
		frame();
		ui->GetPanel<ScenePanel>().OpenCreateNode();
		frame();
		auto* createSearch = static_cast<LineEdit*>(FindNamedUI(root->GetGuiPopup(), "SearchNodeTypes"));
		check(
			createSearch != nullptr && createSearch->HasFocus()
				&& createSearch->GetCursorShape(Vector2::Zero) == CursorShape::IBeam,
			"Create Node focuses a text-cursor search field");
		auto typeKey = [](std::string const& name)
		{
			uint64_t id = 0;
			for (ClassInfo const* info : ClassDatabase::GetRegisteredClasses(true))
			{
				if (info != nullptr && info->m_apiType == ApiType::Runtime
					&& ClassDatabase::IsSubclassOf(info->m_className, "Node"))
				{
					++id;
					if (info->m_className == name)
					{
						return id;
					}
				}
			}
			return uint64_t(0);
		};
		auto* initialTypes = static_cast<Tree*>(FindNamedUI(root->GetGuiPopup(), "NodeTypes"));
		check(
			initialTypes->GetItemRect(typeKey("Node3D")).m_mins.y
					< initialTypes->GetItemRect(typeKey("Camera3D")).m_mins.y
				&& initialTypes->GetItemRect(typeKey("Camera3D")).m_mins.y
					   < initialTypes->GetItemRect(typeKey("Viewport")).m_mins.y,
			"unfiltered type tree keeps a parent's descendants together before the next branch");
		textInput(createSearch, "zzzz-no-node");
		Tree* createTypes = static_cast<Tree*>(FindNamedUI(root->GetGuiPopup(), "NodeTypes"));
		check(createTypes->GetSelected() == 0, "no search match clears stale creation selection");
		key(GLFW_KEY_ENTER);
		check(
			root->GetGuiPopup() != nullptr && creationParent->GetChildren().empty(),
			"Enter with no match cannot create a stale type");
		textInput(createSearch, "Camera3D");
		check(
			createTypes->GetSelected() != 0 && createTypes->GetItemRect(1).GetHeight() > 0.f,
			"search selects a concrete match while retaining its Node ancestor");
		std::function<Button*(Node*, String const&)> findButton = [&](Node* node, String const& text) -> Button*
		{
			if (auto* button = dynamic_cast<Button*>(node);
				button != nullptr && button->GetText() == text && !node->IsQueuedForDeletion())
			{
				return button;
			}

			for (Node* child : node->GetChildren())
			{
				if (Button* result = findButton(child, text))
				{
					return result;
				}
			}
			return nullptr;
		};
		click(findButton(root->GetGuiPopup(), "Favorite"));
		frame();
		check(
			engine.m_fileSystem->Exists("res://.ming/Editor/CreateNodeState.json"),
			"favorites persist in project editor data");
		SaveRenderedFrame(
			engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
			captures / "create-node.ppm");
		check(
			static_cast<Control*>(createTypes->GetParent())->GetGlobalPosition().x
					- static_cast<Control*>(root->GetGuiPopup()->GetChildren().front())->GetLocalPosition().x
				>= editor->GetEditorTheme()->GetConstant("node_sidebar_width", "Editor"),
			"creation sidebar receives editor dimensions before initial layout");
		createSearch->GrabFocus();
		key(GLFW_KEY_ENTER);
		frame();
		check(
			root->GetGuiPopup() == nullptr && creationParent->GetChildren().size() == 1
				&& dynamic_cast<Camera3D*>(creationParent->GetChildren().front()) != nullptr,
			"Enter creates the selected type under the captured parent");
		editor->GetSelection().SetSelected(creationParent->GetObjectID());
		frame();
		ui->GetPanel<ScenePanel>().OpenCreateNode();
		frame();
		createSearch = static_cast<LineEdit*>(FindNamedUI(root->GetGuiPopup(), "SearchNodeTypes"));
		textInput(createSearch, "Node3D");
		creationParent->QueueFree();
		frame();
		key(GLFW_KEY_ENTER);
		check(
			root->GetGuiPopup() != nullptr,
			"deleted creation target leaves modal open instead of falling back to scene root");
		key(GLFW_KEY_ESCAPE);
		frame();

		// 2) Verify real viewport picking, capture, transforms and scene history.
		auto* gizmoTarget = MemNew<MeshInstance3D>();
		gizmoTarget->SetName("GizmoFixture");
		gizmoTarget->SetLocalPosition(Vector3(8.f, 0.f, 0.f));
		scene->AddNode(gizmoTarget);
		editor->GetEditorCamera()->SetLocalPosition(Vector3::Zero);
		editor->GetEditorCamera()->SetLocalOrientation(EulerAngles::Zero);
		editor->GetSelection().SetSelected(gizmoTarget->GetObjectID());
		Camera3D* editCamera = editor->GetEditorCamera()->GetCamera();
		editCamera->SetCurrent();
		EditorGizmos* gizmos = editor->m_editorGizmos;
		gizmos->SetTool(GizmoTool::Move);
		frame();
		frame();
		float const beforeNarrow = ui->GetDockSplit(1)->GetSplitRatio();
		ui->GetDockSplit(1)->SetSplitRatio(0.99f);
		frame();
		frame();
		auto* narrowScript = FindNamedUI(&inspector, "ScriptMenu");
		auto* narrowScroll = FindUIControl<ScrollContainer>(&inspector);
		check(narrowScript != nullptr && narrowScroll != nullptr
			&& narrowScript->GetGlobalPosition().x + narrowScript->GetSize().x
				<= narrowScroll->GetGlobalPosition().x + narrowScroll->GetPageSize().x + 0.01f
			&& narrowScript->GetGlobalPosition().x + narrowScript->GetSize().x <= root->GetOutputResolution().x,
			"Script actions remain inside the visible page at minimum dock width");

		ui->GetDockSplit(1)->SetSplitRatio(beforeNarrow);
		frame();
		frame();
		auto screenPoint = [&](Vector3 const& world)
		{
			Vector2 const size    = sceneView->GetSize();
			float const   tangent = std::tan(editCamera->GetFovDegrees() * 3.14159265f / 360.f);
			Vector3 const cameraPoint = editCamera->GetWorldInverseTransform().TransformPosition3D(world);
			return sceneView->GetGlobalPosition()
				   + Vector2(
					   size.x * 0.5f - cameraPoint.y * size.y / (2.f * cameraPoint.x * tangent),
					   size.y * 0.5f - cameraPoint.z * size.y / (2.f * cameraPoint.x * tangent));
		};
		auto activeDragValue = [&]() -> Label*
		{
			for (Node* child : FindUIControl<TransformGizmo3D>(gizmos)->GetChildren())
			{
				auto* component = dynamic_cast<GizmoComponent*>(child);
				if (component != nullptr && component->IsDragging())
				{
					return static_cast<Label*>(FindNamedUI(component, "GizmoDragValue"));
				}
			}

			return nullptr;
		};
		float const indicatorScale = editor->GetEditorTheme()->GetConstant("icon_size", "Button") / 20.f;
		IntVec2 const sceneSize = sceneView->GetSubViewport()->GetOutputResolution();
		Color const positiveZ = ReadPixel(
			engine.m_renderServer->ViewportGetTexture(sceneView->GetSubViewport()->GetViewportRID()),
			static_cast<unsigned>(sceneSize.x - 80.f * indicatorScale),
			static_cast<unsigned>(80.f * indicatorScale - 25.f));
		check(positiveZ.b > positiveZ.r && positiveZ.b > positiveZ.g,
			"axis indicator connects positive Z above its center when the camera looks along X");
		auto startMove = [&]()
		{
			GizmoContext const context = BuildGizmoContext(&tree, *editCamera);
			Vector2 const      at =
				screenPoint(gizmoTarget->GetWorldPosition() + Vector3(0.f, context.m_scale * 0.95f, 0.f));
			pointer(InputEventType::MouseMotion, at);
			pointer(InputEventType::MouseButton, at, true);
			return at;
		};
		Vector3 const initial = gizmoTarget->GetLocalPosition();
		Vector2       at      = startMove();
		check(gizmos->IsDragging(), "viewport press captures a visible move handle");
		Label* dragValue = static_cast<Label*>(FindNamedUI(gizmos, "GizmoDragValue"));
		check(dragValue != nullptr && dragValue->IsVisible(), "press displays the active transform feedback");
		pointer(InputEventType::MouseMotion, at + Vector2(-35.f, 0.f));
		frame();
		SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), captures / "gizmo-move-drag.ppm");
		pointer(InputEventType::MouseButton, at + Vector2(-35.f, 0.f));
		check(dragValue != nullptr && !dragValue->IsVisible(), "release clears transform feedback");
		check(
			gizmoTarget->GetLocalPosition().y > initial.y && editor->GetUndoRedo().HasUndo(scene->GetObjectID()),
			"move drag changes the constrained axis and records one scene action");
		editor->GetUndoRedo().Undo(scene->GetObjectID());
		check(gizmoTarget->GetLocalPosition() == initial, "scene Undo restores the initial transform");
		editor->GetUndoRedo().Redo(scene->GetObjectID());
		check(gizmoTarget->GetLocalPosition().y > initial.y, "scene Redo restores the committed transform");
		Vector3 const committed = gizmoTarget->GetLocalPosition();
		frame();
		at = startMove();
		pointer(InputEventType::MouseMotion, at + Vector2(-20.f, 0.f));
		key(GLFW_KEY_ESCAPE);
		pointer(InputEventType::MouseButton, at);
		check(
			!gizmos->IsDragging() && gizmoTarget->GetLocalPosition() == committed,
			"Escape restores drag snapshot without committing");
		check(dragValue != nullptr && !dragValue->IsVisible(), "Escape clears transform feedback");
		frame();
		at = startMove();
		pointer(InputEventType::MouseMotion, at + Vector2(-20.f, 0.f));
		InputEvent lostFocus;
		lostFocus.m_type = InputEventType::WindowFocus;
		root->ProcessInputEvent(lostFocus);
		check(
			!gizmos->IsDragging() && gizmoTarget->GetLocalPosition() == committed,
			"window focus loss cancels the captured transform");
		check(dragValue != nullptr && !dragValue->IsVisible(), "focus loss clears transform feedback");
		lostFocus.m_focused = true;
		root->ProcessInputEvent(lostFocus);
		frame();
		at = startMove();
		pointer(InputEventType::MouseMotion, at + Vector2(-20.f, 0.f));
		ui->Warning("Transform Interrupted", "A modal cancels the active transform.");
		check(
			!gizmos->IsDragging() && gizmoTarget->GetLocalPosition() == committed,
			"opening a modal cancels the transform before its input starts");
		key(GLFW_KEY_ESCAPE);
		frame();
		at                    = startMove();
		Vector2 const outside = sceneView->GetGlobalPosition() + Vector2(-20.f, sceneView->GetSize().y * 0.5f);
		pointer(InputEventType::MouseMotion, outside);
		pointer(InputEventType::MouseButton, outside);
		check(
			!gizmos->IsDragging() && gizmoTarget->GetLocalPosition() != committed,
			"release outside viewport commits the captured transform");
		editor->GetUndoRedo().Undo(scene->GetObjectID());
		check(gizmoTarget->GetLocalPosition() == committed, "outside-release operation remains one undoable transform");
		gizmoTarget->SetLocalOrientation(EulerAngles(35.f, 20.f, 10.f));
		gizmos->SetLocal(true);
		GizmoContext const local = BuildGizmoContext(&tree, *editCamera);
		check(local.m_basis.GetIBasis3D() != Vector3::Forward, "Local gizmo basis follows the rotated target");
		gizmos->SetLocal(false);
		check(
			BuildGizmoContext(&tree, *editCamera).m_basis.GetIBasis3D() == Vector3::Forward,
			"World gizmo retains fixed axes");
		gizmos->SetTool(GizmoTool::Scale);
		frame();
		Vector3 const initialScale = gizmoTarget->GetLocalScale();
		at                         = screenPoint(gizmoTarget->GetWorldPosition());
		pointer(InputEventType::MouseButton, at, true);
		pointer(InputEventType::MouseMotion, at + Vector2(30.f, -30.f));
		frame();
		Label* scaleValue = activeDragValue();
		check(scaleValue != nullptr && scaleValue->IsVisible() && scaleValue->GetText() != String("x 1.000"),
			"uniform scaling displays the changing scale factor");
		SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), captures / "gizmo-scale-drag.ppm");
		pointer(InputEventType::MouseButton, at + Vector2(30.f, -30.f));
		check(gizmoTarget->GetLocalScale().x > initialScale.x, "center scale handle changes uniform scale");
		gizmos->SetTool(GizmoTool::Rotate);
		frame();
		GizmoContext const rotationPreview = BuildGizmoContext(&tree, *editCamera);
		at = screenPoint(gizmoTarget->GetWorldPosition() + Vector3::Left * (rotationPreview.m_scale * 0.8f));
		pointer(InputEventType::MouseMotion, at);
		pointer(InputEventType::MouseButton, at, true);
		Vector2 const rotatedPoint = screenPoint(
			gizmoTarget->GetWorldPosition() + Vector3::Up * (rotationPreview.m_scale * 0.8f));
		pointer(InputEventType::MouseMotion, rotatedPoint);
		frame();
		Label* angleValue = activeDragValue();
		check(angleValue != nullptr && angleValue->IsVisible()
				&& std::abs(std::stof(angleValue->GetText().ToUtf8()) - 90.f) < 1.f,
			"rotation drag displays the swept quarter-turn angle");
		SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()), captures / "gizmo-rotate-drag.ppm");
		key(GLFW_KEY_ESCAPE);
		pointer(InputEventType::MouseButton, rotatedPoint);
		check(angleValue != nullptr && !angleValue->IsVisible(), "cancelling rotation removes its angle and sector feedback");

		// 3) Verify camera-facing plane picking and continuous display scaling during a real drag.
		Vector3 const planeFixturePosition = gizmoTarget->GetLocalPosition();
		EulerAngles const planeFixtureOrientation = gizmoTarget->GetLocalOrientation();
		gizmoTarget->SetLocalPosition(Vector3::Zero);
		gizmos->SetTool(GizmoTool::Move);
		for (bool localMode : { false, true })
		{
			gizmos->SetLocal(localMode);
			for (int octant = 0; octant < 8; ++octant)
			{
				Vector3 const cameraPosition((octant & 1) ? -6.f : 6.f,
					(octant & 2) ? -6.f : 6.f, (octant & 4) ? -6.f : 6.f);
				editor->GetEditorCamera()->SetLocalPosition(cameraPosition);
				editor->GetEditorCamera()->SetWorldForward(-cameraPosition);
				frame();
				GizmoContext const facing = BuildGizmoContext(&tree, *editCamera);
				bool facesCamera = true;
				bool picksVisibleCenter = true;
				for (Node* child : FindUIControl<TransformGizmo3D>(gizmos)->GetChildren())
				{
					auto* component = dynamic_cast<GizmoComponent*>(child);
					if (component == nullptr || component->GetKind() != GizmoKind::Plane)
					{
						continue;
					}

					Vector3 const center = component->GetWorldVirtualCenter();
					facesCamera = facesCamera && Vector3::DotProduct(center, cameraPosition) > 0.f;
					MathRaycastQuery3D const ray{ cameraPosition, (center - cameraPosition).GetNormalized(), 100.f };
					picksVisibleCenter = picksVisibleCenter && component->Raycast(facing, ray).m_didImpact;
				}

				check(facesCamera, "World and Local plane handles face the camera in every viewing octant");
				check(picksVisibleCenter, "plane hit regions follow the rendered mirrored centers");
			}
		}

		gizmos->SetLocal(false);
		gizmoTarget->SetLocalOrientation(EulerAngles::Zero);
		Vector3 const planeCameraPosition(6.f, -6.f, 6.f);
		editor->GetEditorCamera()->SetLocalPosition(planeCameraPosition);
		editor->GetEditorCamera()->SetWorldForward(-planeCameraPosition);
		editCamera->SetOrthogonal(10.f, 0.f, 100.f);
		check(BuildGizmoContext(&tree, *editCamera).m_planeSigns == Vector3::One,
			"orthographic plane handles retain their unmirrored layout");
		editCamera->SetPerspective(60.f, 0.1f, 100.f);
		frame();
		GizmoComponent* planeHandle = nullptr;
		for (Node* child : FindUIControl<TransformGizmo3D>(gizmos)->GetChildren())
		{
			auto* component = dynamic_cast<GizmoComponent*>(child);
			if (component != nullptr && component->GetKind() == GizmoKind::Plane
				&& std::abs(component->GetWorldVirtualCenter().z) < 0.0001f)
			{
				planeHandle = component;
			}
		}

		check(planeHandle != nullptr, "XY plane fixture has a visible handle");
		if (planeHandle != nullptr)
		{
			Vector3 const startCenter = planeHandle->GetWorldVirtualCenter();
			float const startSize = planeHandle->GetWorldScale().x;
			at = screenPoint(startCenter);
			pointer(InputEventType::MouseMotion, at);
			pointer(InputEventType::MouseButton, at, true);
			check(gizmos->IsDragging() && planeHandle->IsDragging(), "mirrored plane press captures that plane");
			pointer(InputEventType::MouseMotion, screenPoint(startCenter + Vector3(1.5f, 0.5f, 0.f)));
			frame();
			SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
				captures / "gizmo-plane-drag.ppm");
			Vector2 const planeEnd = screenPoint(startCenter + Vector3(12.f, 0.f, 0.f));
			pointer(InputEventType::MouseMotion, planeEnd);
			frame();
			GizmoContext const moved = BuildGizmoContext(&tree, *editCamera);
			check(gizmoTarget->GetWorldPosition().x > 6.f && std::abs(gizmoTarget->GetWorldPosition().z) < 0.0001f,
				"plane translation remains constrained while crossing a facing boundary");
			check(planeHandle->GetWorldTransform().GetIBasis3D().x > 0.f && moved.m_planeSigns.x < 0.f,
				"active plane keeps its starting quadrant while the object crosses the camera");
			check(std::abs(planeHandle->GetWorldScale().x / moved.m_scale - 1.f) < 0.001f
					&& std::abs(planeHandle->GetWorldScale().x - startSize) > 0.1f,
				"dragging updates the displayed size using the current object depth");
			Label* planeValue = activeDragValue();
			check(planeValue != nullptr && planeValue->IsVisible() && planeValue->GetText().Contains(U'X')
					&& planeValue->GetText().Contains(U'Y'),
				"plane feedback displays both constrained displacement components");
			editor->GetEditorCamera()->SetLocalPosition(planeCameraPosition * 1.5f);
			frame();
			float const currentSize = planeHandle->GetWorldScale().x;
			check(std::abs(currentSize / BuildGizmoContext(&tree, *editCamera).m_scale - 1.f) < 0.001f,
				"camera distance changes refresh the active display without another mouse event");
			pointer(InputEventType::MouseButton, planeEnd);
			frame();
			check(!gizmos->IsDragging() && std::abs(planeHandle->GetWorldScale().x - currentSize) < 0.001f,
				"releasing a depth-changing drag does not jump in size");
			editor->GetUndoRedo().Undo(scene->GetObjectID());
			check(gizmoTarget->GetWorldPosition().GetLengthSquared() < 0.0001f,
				"a depth-changing plane drag remains one undoable transform");
		}

		gizmoTarget->SetLocalPosition(planeFixturePosition);
		gizmoTarget->SetLocalOrientation(planeFixtureOrientation);
		editor->GetEditorCamera()->SetLocalPosition(Vector3::Zero);
		editor->GetEditorCamera()->SetLocalOrientation(EulerAngles::Zero);
		frame();
		// 4) Exercise rotated parents and TRS limits without depending on another test's state.
		auto* mathParent = MemNew<Node3D>();
		mathParent->SetLocalOrientation(EulerAngles(25.f, 10.f, 15.f));
		scene->AddNode(mathParent);
		auto* mathTarget = MemNew<Node3D>();
		mathParent->AddNode(mathTarget);
		mathTarget->SetLocalOrientation(EulerAngles(35.f, 20.f, 10.f));
		mathTarget->SetLocalScale(Vector3(1.f, 2.f, 3.f));
		GizmoContext rotationContext;
		rotationContext.m_camera         = editCamera;
		rotationContext.m_sceneTree      = &tree;
		rotationContext.m_selectedNode3D = mathTarget;
		rotationContext.m_originWorld    = mathTarget->GetWorldPosition();
		Vector3 const beforeForward      = mathTarget->GetWorldForward();
		auto*         rotationHandle     = MemNew<GizmoRotationArc>(GizmoAxis::X, Color::White);
		rotationHandle->OnBeginDrag(rotationContext, rotationContext.m_originWorld + Vector3::Left);
		MathRaycastQuery3D quarterTurn{ rotationContext.m_originWorld + Vector3(10.f, 0.f, 1.f),
										-Vector3::Forward,
										100.f };
		rotationHandle->OnDrag(rotationContext, quarterTurn, quarterTurn);
		Vector3 const expectedForward(beforeForward.x, -beforeForward.z, beforeForward.y);
		check(
			(mathTarget->GetWorldForward() - expectedForward).GetLengthSquared() < 0.00001f
				&& mathTarget->GetLocalScale() == Vector3(1.f, 2.f, 3.f),
			"World rotation composes around the selected axis through a rotated parent");
		mathParent->SetLocalScale(Vector3(2.f, 3.f, 1.f));
		rotationContext.m_local   = true;
		rotationContext.m_basis   = mathTarget->GetWorldOrientation().GetAsMatrix_IFwd_JLeft_KUp();
		Vector3 const axis        = rotationContext.m_basis.GetIBasis3D();
		Vector3 const initialAxis = mathTarget->GetWorldTransform().GetIBasis3D().GetNormalized();
		rotationHandle->OnBeginDrag(
			rotationContext,
			rotationContext.m_originWorld + rotationContext.m_basis.GetJBasis3D());
		quarterTurn = { rotationContext.m_originWorld + rotationContext.m_basis.GetKBasis3D() + axis * 10.f,
						-axis,
						100.f };
		rotationHandle->OnDrag(rotationContext, quarterTurn, quarterTurn);
		check(
			(mathTarget->GetWorldTransform().GetIBasis3D().GetNormalized() - initialAxis).GetLengthSquared() < 0.00001f
				&& mathTarget->GetLocalScale() == Vector3(1.f, 2.f, 3.f),
			"Local rotation preserves its axis and scale under a nonuniform parent");
		MemDelete(rotationHandle);
		mathTarget->SetLocalScale(Vector3(-1.f, -2.f, 3.f));
		GizmoContext scaleContext;
		scaleContext.m_camera         = editCamera;
		scaleContext.m_selectedNode3D = mathTarget;
		scaleContext.m_local          = true;
		scaleContext.m_scale          = 1.f;
		auto* scaleHandle             = MemNew<GizmoAxisArrow>(GizmoAxis::Y, Color::White, GizmoKind::Scale);
		scaleHandle->OnBeginDrag(scaleContext, Vector3::Left);
		MathRaycastQuery3D scaleStart{ Vector3(10.f, 1.f, 0.f), -Vector3::Forward, 100.f };
		MathRaycastQuery3D scaleEnd{ Vector3(10.f, -100.f, 0.f), -Vector3::Forward, 100.f };
		scaleHandle->OnDrag(scaleContext, scaleStart, scaleEnd);
		check(
			mathTarget->GetLocalScale().x == -1.f && mathTarget->GetLocalScale().y < 0.f
				&& std::abs(mathTarget->GetLocalScale().y) >= 0.0001f,
			"axis scaling preserves signs and cannot cross a singular zero scale");
		MemDelete(scaleHandle);
		mathParent->QueueFree();
		gizmos->SetTool(GizmoTool::Transform);
		frame();
		SaveRenderedFrame(
			engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
			captures / "viewport-gizmos.ppm");
		gizmoTarget->QueueFree();
		editor->GetSelection().SetSelected(camera->GetObjectID());
		frame();
		check(
			EditorIcons::GetIconTexture("Search")->GetDimensions() == IntVec2(32, 32)
				&& EditorIcons::GetIconTexture("Search")->m_samplerMode == SamplerMode::BILINEAR_CLAMP,
			"editor icons keep 32px sources with bilinear sampling");

		auto* colorLight = MemNew<OmniLight3D>();
		colorLight->SetName("ColorPickerPreview");
		Color const originalColor(200, 120, 60, 128);
		colorLight->SetColor(originalColor);
		scene->AddNode(colorLight);
		editor->GetSelection().SetSelected(colorLight->GetObjectID());
		frame();
		auto* colorButton = FindUIControl<ColorPickerButton>(&inspector);
		check(colorButton != nullptr, "Inspector uses a clickable color field for Light3D");
		if (colorButton != nullptr)
		{
			check(
				editor->SaveSceneAs(VirtualPath("res://Scenes/ColorPickerVerification.tscn")),
				"color workflow saves its own baseline scene");
			editor->GetUndoRedo().ClearHistory(scene->GetObjectID());
			click(colorButton);
			colorButton->ClosePicker(true);
			check(
				!editor->IsSceneDirty() && !editor->GetUndoRedo().Undo(scene->GetObjectID()),
				"unchanged picker session creates neither dirty state nor undo action");
			click(colorButton);
			frame();
			auto* picker = colorButton->GetPicker();
			ColorPickerVerification::EnterHex(root, picker, "#12345678");
			frame();
			check(
				colorLight->GetColor() == Color(18, 52, 86, 120) && !editor->IsSceneDirty(),
				"color drag previews getter value without dirtying scene");
			check(!editor->GetUndoRedo().Undo(scene->GetObjectID()), "color preview does not add undo actions");
			ColorPickerVerification::EnterHex(root, picker, "#00000000");
			frame();
			float const checker = static_cast<float>(picker->GetThemeConstant("checker_size", "ColorPicker"));
			AABB2 const oldPreview = picker->GetOldColorRect();
			Vector2 const sample = picker->GetGlobalPosition() + Vector2(oldPreview.m_maxs.x + checker * 0.5f, oldPreview.m_mins.y + checker * 0.5f);
			GPUTexture* output = engine.m_renderServer->ViewportGetTexture(root->GetViewportRID());
			check(ReadPixel(output, static_cast<unsigned>(sample.x), static_cast<unsigned>(sample.y)) == picker->GetThemeColor("checker_light", "ColorPicker")
				&& ReadPixel(output, static_cast<unsigned>(sample.x + checker), static_cast<unsigned>(sample.y)) == picker->GetThemeColor("checker_dark", "ColorPicker"),
				"transparent preview renders the scaled checker background");
			ColorPickerVerification::EnterHex(root, picker, "#98765432");
			frame();
			SaveRenderedFrame(
				engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
				captures / "inspector-color-picker.ppm");
			colorButton->ClosePicker(true);
			check(
				editor->IsSceneDirty() && colorLight->GetColor() == Color(152, 118, 84, 50),
				"closing color picker commits actual value");
			check(
				editor->GetUndoRedo().Undo(scene->GetObjectID()) && colorLight->GetColor() == originalColor,
				"one undo restores color before full picker session");
			check(!editor->GetUndoRedo().Undo(scene->GetObjectID()), "multiple previews create only one undo action");
			check(
				editor->GetUndoRedo().Redo(scene->GetObjectID()) && colorLight->GetColor() == Color(152, 118, 84, 50),
				"redo restores final color");
			frame();
			colorButton->OpenPicker();
			ColorPickerVerification::EnterHex(root, picker, "#ffffffff");
			colorLight->SetColor(Color(6, 7, 8, 9));
			frame();
			check(
				!colorButton->IsPickerOpen() && colorLight->GetColor() == Color(6, 7, 8, 9),
				"external property change closes color session without overwriting new value");
			colorButton->OpenPicker();
			ColorPickerVerification::EnterHex(root, picker, "#ffffffff");
			key(GLFW_KEY_ESCAPE);
			check(colorLight->GetColor() == Color(6, 7, 8, 9), "Escape restores original property through setter");
			colorButton->OpenPicker();
			ColorPickerVerification::EnterHex(root, picker, "#10203040");
			editor->GetSelection().SetSelected(camera->GetObjectID());
			frame();
			check(
				root->GetGuiPopup() == nullptr && colorLight->GetColor() == Color(6, 7, 8, 9),
				"selection switch rolls back color preview and closes popup");
			editor->GetSelection().SetSelected(colorLight->GetObjectID());
			frame();
			colorButton = FindUIControl<ColorPickerButton>(&inspector);
			colorButton->OpenPicker();
			ColorPickerVerification::EnterHex(root, colorButton->GetPicker(), "#10203040");
			colorLight->QueueFree();
			frame();
			check(root->GetGuiPopup() == nullptr, "deleting selected color target closes popup safely");
		}

		editor->GetSelection().SetSelected(camera->GetObjectID());
		frame();

		if (!preview)
		{
			tree.ClearScene();
			tree.FlushPendingNode();
			editor->GetSelection().Clear();
			frame();
			ui->GetPanel<ScenePanel>().OpenCreateNode();
			frame();
			key(GLFW_KEY_ENTER);
			frame();
			check(
				tree.GetScene() != nullptr && tree.GetScene()->GetName() == String("Node") && editor->IsSceneDirty(),
				"creating the first scene root attaches it before marking it unsaved");
		}

		if (preview)
		{
			std::vector<Vertex> vertices;
			AddVertsForAABB3D(
				vertices,
				AABB3(Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f)),
				Color(155, 143, 125));
			SurfaceTool surface;
			for (Vertex const& vertex : vertices)
			{
				surface.SetColor(vertex.m_color);
				surface.SetUV(vertex.m_uv);
				surface.AddVertex(vertex.m_position);
			}

			surface.GenerateNormals();
			surface.Index();
			auto mesh                                  = surface.CreateMeshResource();
			mesh->m_materialResource                   = CreateRef<MaterialResource>();
			mesh->m_materialResource->m_shaderResource = engine.m_renderServer->GetDefaultUnlitShaderResource();
			mesh->InitGPUResources();
			auto* previewTarget = MemNew<MeshInstance3D>();
			previewTarget->SetName("GizmoPreview");
			previewTarget->SetMeshResource(mesh);
			previewTarget->SetLocalPosition(Vector3(8.f, 0.f, 0.f));
			scene->AddNode(previewTarget);
			editor->GetEditorCamera()->SetLocalPosition(Vector3(0.f, 0.f, 3.f));
			editor->GetEditorCamera()->SetLocalOrientation(EulerAngles(0.f, 20.f, 0.f));
			editor->GetEditorCamera()->GetCamera()->SetCurrent();
			auto* previewLight = MemNew<OmniLight3D>();
			previewLight->SetName("ColorPickerPreview");
			previewLight->SetColor(Color(211, 173, 114, 180));
			scene->AddNode(previewLight);
			editor->GetSelection().SetSelected(previewLight->GetObjectID());
			auto* window = engine.m_windowSystem->GetGLFWWindow();
			glfwSetKeyCallback(window, keyCallback);
			glfwSetMouseButtonCallback(window, mouseButtonCallback);
			glfwSetCursorPosCallback(window, cursorPosCallback);
			glfwSetCharCallback(window, characterCallback);
			glfwSetScrollCallback(window, scrollCallback);
			glfwSetWindowFocusCallback(window, windowFocusCallback);
			glfwSetCursorEnterCallback(window, cursorEnterCallback);
			glfwSetFramebufferSizeCallback(window, framebufferCallback);
			glfwSetWindowTitle(window, "Ming Editor Interaction Preview");
			glfwRestoreWindow(window);
			glfwSetWindowSize(window, 1440, 900);
			while (!glfwWindowShouldClose(window))
			{
				int width  = 0;
				int height = 0;
				glfwGetFramebufferSize(window, &width, &height);
				root->UpdateResolution(IntVec2(width, height));
				frame();
				std::this_thread::sleep_for(std::chrono::milliseconds(16));
			}
		}
	}

	ProjectSettings::Reset();
	ThemeDatabase::Shutdown();
	ResourceLoader::Shutdown();
	EditorIcons::Shutdown();
	engine.Shutdown();
	ClassDatabase::Shutdown();
	std::printf("EDITOR_UI checks=%d failures=%d\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
