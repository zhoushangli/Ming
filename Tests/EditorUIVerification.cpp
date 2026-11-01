#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
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
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "RenderReadback.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>

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

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		return 2;
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
		auto theme = EditorThemeManager::CreateTheme();
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
			return std::vector<MenuBar::MenuItem>{ { "Disabled", {} }, { "Action", [&]() { ++actions; } } };
		});
		bar->AddMenu("Second", []()
		{
			return std::vector<MenuBar::MenuItem>{ { "Unavailable", {} } };
		});
		bar->UpdateSize(bar->GetCombinedMinimumSize());
		tree.FlushContainerUpdate();
		auto* first = static_cast<Button*>(bar->GetChildren()[0]);
		auto* second = static_cast<Button*>(bar->GetChildren()[1]);
		check(first->GetFontSize() == 15 && bar->GetSize().y >= 35.f && first->GetSize().y == bar->GetSize().y,
			"editor menu fonts and title hit areas use configured menu height");
		click(first);
		check(bar->GetActiveMenu() == 0 && root->GetGuiPopup() != nullptr && first->IsPressed(),
			"click opens menu and marks its title active");
		auto* focused = dynamic_cast<Button*>(root->GetGuiFocus());
		check(focused != nullptr && focused->GetText() == String("Action"), "menu initially skips disabled items");
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
		scope->QueueFree();
		tree.FlushPendingNode();

		auto* editor = MemNew<EditorNode>();
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
		check(productionBar != nullptr && ui->GetDockSplit(0)->GetGlobalPosition().y == 60.f,
			"production editor uses MenuBar and the configured top bar height");
		check(editor->GetEditorTheme()->GetDefaultFontSize() == 15
			&& ThemeDatabase::GetDefaultTheme()->GetDefaultFontSize() == 16,
			"fixed editor dimensions leave runtime theme unchanged");
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

		std::filesystem::path const captures = project.parent_path() / "Captures";
		std::filesystem::create_directories(captures);
		for (IntVec2 size : { IntVec2(1440, 900), IntVec2(960, 600) })
		{
			root->UpdateResolution(size);
			frame();
			frame();
			SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(root->GetViewportRID()),
				captures / ("editor-ui-" + std::to_string(size.x) + ".ppm"));
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
