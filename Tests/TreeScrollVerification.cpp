#define NOMINMAX
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "ThirdParty/GLFW/glfw3.h"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/CreateNodePopup.hpp"
#include "MingEngine/Editor/UI/Popup/SceneFilePopup.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Render/GPUTexture.hpp"
#include <d3d11.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <limits>
#include <stdexcept>

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

bool Near(float a, float b)
{
	return std::abs(a - b) < 0.01f;
}

void Click(Viewport* viewport, Vector2 position)
{
	InputEvent event;
	event.m_position = position;
	event.m_type = InputEventType::MouseMotion;
	viewport->ProcessInputEvent(event);
	event.m_type = InputEventType::MouseButton;
	event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
	event.m_pressed = true;
	viewport->ProcessInputEvent(event);
	event.m_pressed = false;
	viewport->ProcessInputEvent(event);
}

void Frame(Engine& engine)
{
	engine.BeginFrame();
	engine.Update(1.f / 60.f);
	engine.Render();
	engine.EndFrame();
}

void Verify(Engine& engine, bool preview)
{
	auto* viewport = static_cast<Viewport*>(engine.m_sceneSystem->GetSceneTree()->GetRoot());
	auto* tree = MemNew<Tree>();
	tree->SetTheme(EditorThemeManager::CreateTheme());
	tree->SetName("ScrollingTree");
	tree->SetPosition(Vector2(20.f, 20.f));
	tree->SetClipText(true);
	viewport->AddNode(tree);
	tree->UpdateSize(Vector2(300.f, 240.f));
	std::vector<Tree::Item> items;
	items.push_back({ 1, 0, "Root", true, true });
	for (uint64_t i = 2; i <= 80; ++i)
	{
		items.push_back({ i, 1, "A long tree item that extends well beyond the visible width of the panel", true, true });
	}
	items[19].m_actions.push_back({ {}, Color::White, true });
	tree->SetItems(items);
	auto* other = MemNew<LineEdit>();
	viewport->AddNode(other);
	other->SetPosition(Vector2(350.f, 20.f));
	other->UpdateSize(Vector2(200.f, 32.f));
	Frame(engine);
	InputEvent focus;
	focus.m_type = InputEventType::WindowFocus;
	focus.m_focused = true;
	viewport->ProcessInputEvent(focus);
	Check(tree->GetCombinedMinimumSize().y < 240.f, "tree minimum height is independent of content");
	Check(tree->GetContentSize().y > tree->GetPageSize().y, "tree owns vertical overflow");
	other->GrabFocus();
	Vector2 point(90.f, tree->GetItemRect(2).GetCenter().y);
	Click(viewport, tree->GetGlobalPosition() + point);
	Check(viewport->GetGuiFocus() == tree && tree->GetSelected() == 2, "focus click selects the original visible row");
	Check(Near(tree->GetScroll(), 0.f), "focus click preserves the scroll position");

	InputEvent wheel;
	wheel.m_type = InputEventType::MouseWheel;
	wheel.m_position = tree->GetGlobalPosition() + Vector2(90.f, 80.f);
	wheel.m_scroll = Vector2(0.f, -2.f);
	viewport->ProcessInputEvent(wheel);
	Check(tree->GetScroll() > 0.f, "tree handles viewport wheel input");
	uint64_t hit = tree->GetItemAtPosition(Vector2(90.f, 80.f));
	Click(viewport, wheel.m_position);
	Check(tree->GetSelected() == hit, "scrolled hit testing matches item geometry");
	tree->Select(80);
	Check(tree->GetItemRect(80).m_maxs.y <= tree->GetPageSize().y + 0.01f, "selection scrolls into view");
	InputEvent key;
	key.m_type = InputEventType::Key;
	key.m_pressed = true;
	key.m_keyCode = GLFW_KEY_UP;
	viewport->ProcessInputEvent(key);
	Check(tree->GetSelected() == 79, "keyboard navigation retains tree focus");
	tree->EditSelected();
	auto* edit = dynamic_cast<LineEdit*>(viewport->GetGuiFocus());
	Check(edit != nullptr && Near(edit->GetLocalPosition().y, tree->GetItemRect(79).m_mins.y), "rename editor follows scrolled row");
	if (edit != nullptr)
	{
		edit->EndEditing(LineEdit::EditEndReason::Cancelled);
	}

	tree->SetScroll(0.f);
	uint64_t selected = tree->GetSelected();
	Vector2 trackPoint(tree->GetPageSize().x + 2.f, tree->GetPageSize().y * 0.7f);
	Click(viewport, tree->GetGlobalPosition() + trackPoint);
	Check(tree->GetScroll() > 0.f && tree->GetSelected() == selected, "scrollbar track click scrolls without selecting");
	InputEvent press;
	press.m_type = InputEventType::MouseButton;
	press.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
	press.m_pressed = true;
	press.m_position = tree->GetGlobalPosition() + trackPoint;
	viewport->ProcessInputEvent(press);
	press.m_type = InputEventType::MouseMotion;
	press.m_position.y = tree->GetGlobalPosition().y + 25.f;
	viewport->ProcessInputEvent(press);
	press.m_type = InputEventType::MouseButton;
	press.m_pressed = false;
	viewport->ProcessInputEvent(press);
	Check(tree->GetScroll() < tree->GetContentSize().y * 0.5f, "scrollbar drag changes the offset");

	tree->SetClipText(false);
	Check(tree->GetContentSize().x > tree->GetPageSize().x, "unclipped text enables horizontal overflow");
	selected = tree->GetSelected();
	Click(viewport, tree->GetGlobalPosition() + tree->GetPageSize() + Vector2(2.f, 2.f));
	Check(tree->GetSelected() == selected, "scrollbar corner does not clear selection");
	wheel.m_modifiers = GLFW_MOD_SHIFT;
	viewport->ProcessInputEvent(wheel);
	Check(tree->GetHorizontalScroll() > 0.f, "shift wheel scrolls horizontally");
	tree->SetHorizontalScroll(std::numeric_limits<float>::infinity());
	Check(std::isfinite(tree->GetHorizontalScroll()), "invalid offsets are ignored");
	tree->SetClipText(true);
	Check(Near(tree->GetHorizontalScroll(), 0.f), "text clipping removes horizontal overflow");
	tree->Select(20);
	int actions = 0;
	tree->SetItemActionCallback([&actions](uint64_t id, int action)
	{
		if (id == 20 && action == 0)
		{
			++actions;
		}
	});
	Click(viewport, tree->GetGlobalPosition() + tree->GetItemActionRect(20, 0).GetCenter());
	Check(actions == 1 && tree->GetSelected() == 20, "scrolled action geometry routes the correct callback");
	bool scrolledDrag = false;
	tree->SetDragCallbacks([&scrolledDrag](Vector2 const&, GuiDragData& data)
	{
		scrolledDrag = true;
		data.m_type = "test";
		return true;
	}, {}, {});
	GuiDragData scrolledData;
	Check(tree->GetDragData(Vector2(90.f, tree->GetItemRect(20).GetCenter().y), scrolledData) && scrolledDrag,
		"scrolled item supports drag data");
	tree->SetScroll(0.f);
	InputEvent drag;
	drag.m_type = InputEventType::MouseButton;
	drag.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
	drag.m_pressed = true;
	drag.m_position = tree->GetGlobalPosition() + Vector2(tree->GetPageSize().x + 2.f, 10.f);
	viewport->ProcessInputEvent(drag);
	tree->GetTheme()->SetConstant("scrollbar_width", "ScrollContainer", 18);
	float const beforeMove = tree->GetScroll();
	drag.m_type = InputEventType::MouseMotion;
	drag.m_position.y += 80.f;
	viewport->ProcessInputEvent(drag);
	Check(Near(tree->GetScroll(), beforeMove) && Near(tree->GetSize().x - tree->GetPageSize().x, 18.f),
		"live scrollbar geometry refresh cancels obsolete drag");
	drag.m_type = InputEventType::MouseButton;
	drag.m_pressed = false;
	viewport->ProcessInputEvent(drag);
	tree->GetTheme()->SetColor("font_color", "Tree", Color::White);
	Check(Near(tree->GetScroll(), beforeMove), "color theme refresh preserves scroll position");
	tree->SetCollapsed(1, true);
	Check(Near(tree->GetScroll(), 0.f), "collapse clamps scroll range");
	tree->SetCollapsed(1, false);
	tree->SetScroll(10000.f);
	tree->UpdateSize(Vector2(300.f, 400.f));
	Check(tree->GetScroll() <= tree->GetContentSize().y - tree->GetPageSize().y + 0.01f, "resize clamps offset");
	tree->SetItems({ { 1, 0, "Filtered" } }, true);
	Check(Near(tree->GetScroll(), 0.f), "filtered content clamps offset");
	tree->SetItems(items);
	tree->UpdateSize(Vector2(300.f, 240.f));
	tree->SetScroll(0.f);
	bool dragged = false;
	tree->SetDragCallbacks([&dragged](Vector2 const&, GuiDragData& data)
	{
		dragged = true;
		data.m_type = "test";
		return true;
	}, {}, {});
	GuiDragData data;
	Check(tree->GetDragData(Vector2(90.f, tree->GetItemRect(2).GetCenter().y), data) && dragged, "visible item supports drag data");
	dragged = false;
	Check(!tree->GetDragData(trackPoint, data) && !dragged, "scrollbar cannot initiate item dragging");

	auto* scroll = MemNew<ScrollContainer>();
	viewport->AddNode(scroll);
	scroll->SetPosition(Vector2(350.f, 100.f));
	scroll->UpdateSize(Vector2(250.f, 180.f));
	auto* content = MemNew<Control>();
	content->SetCustomMinimumSize(Vector2(500.f, 800.f));
	scroll->AddNode(content);
	content->SetFocusMode(Control::FocusMode::All);
	auto* field = MemNew<LineEdit>();
	content->AddNode(field);
	field->SetPosition(Vector2(20.f, 600.f));
	field->UpdateSize(Vector2(100.f, 32.f));
	Frame(engine);
	Check(!scroll->IsFollowingFocus(), "follow focus defaults to disabled");
	field->GrabFocus();
	Check(Near(scroll->GetScroll(), 0.f), "disabled follow focus does not scroll");
	scroll->EnsureControlVisible(field);
	Check(scroll->GetScroll() > 0.f, "explicit visibility ignores follow focus setting");
	scroll->SetScroll(0.f);
	scroll->SetFollowFocus(true);
	other->GrabFocus();
	field->GrabFocus();
	Check(scroll->GetScroll() > 0.f, "enabled follow focus reveals fields");
	scroll->SetScroll(100.f);
	scroll->SetHorizontalScroll(80.f);
	scroll->EnsureControlVisible(content);
	Check(Near(scroll->GetScroll(), 100.f) && Near(scroll->GetHorizontalScroll(), 80.f), "oversized target covering viewport preserves both offsets");
	scroll->SetScroll(0.f);
	other->GrabFocus();
	content->GrabFocus();
	Check(Near(scroll->GetScroll(), 0.f), "oversized focused content does not jump to bottom");
	field->SetPosition(Vector2(20.f, -20.f));
	scroll->EnsureControlVisible(field);
	Check(Near(scroll->GetScroll(), 0.f), "visibility adjustment clamps at zero");
	field->SetPosition(Vector2(20.f, 600.f));
	auto* inner = MemNew<ScrollContainer>();
	content->AddNode(inner);
	inner->SetPosition(Vector2(20.f, 300.f));
	inner->UpdateSize(Vector2(180.f, 100.f));
	inner->SetFollowFocus(true);
	auto* innerContent = MemNew<Control>();
	innerContent->SetCustomMinimumSize(Vector2(180.f, 500.f));
	inner->AddNode(innerContent);
	auto* innerField = MemNew<LineEdit>();
	innerContent->AddNode(innerField);
	innerField->SetPosition(Vector2(5.f, 400.f));
	innerField->UpdateSize(Vector2(100.f, 32.f));
	Frame(engine);
	other->GrabFocus();
	innerField->GrabFocus();
	Vector2 const innerPosition = innerField->GetGlobalPosition() - inner->GetGlobalPosition();
	Vector2 const outerPosition = innerField->GetGlobalPosition() - scroll->GetGlobalPosition();
	Check(innerPosition.y >= 0.f && innerPosition.y + innerField->GetSize().y <= inner->GetPageSize().y + 0.01f,
		"nested focus reveals field in inner viewport");
	Check(outerPosition.y >= 0.f && outerPosition.y + innerField->GetSize().y <= scroll->GetPageSize().y + 0.01f,
		"nested focus uses updated geometry in outer viewport");
	Frame(engine);
	if (preview)
	{
		while (!glfwWindowShouldClose(engine.m_windowSystem->GetGLFWWindow()))
		{
			Frame(engine);
		}
	}
}

void CaptureRenderedFrame(Engine& engine, std::string const& path)
{
	auto* viewport = static_cast<Viewport*>(engine.m_sceneSystem->GetSceneTree()->GetRoot());
	auto* texture  = engine.m_renderServer->ViewportGetTexture(viewport->GetViewportRID());
	Check(texture != nullptr, "rendered viewport texture exists");
	ID3D11Resource* resource = nullptr;
	texture->GetShaderResourceView()->GetResource(&resource);
	ID3D11Texture2D* source = nullptr;
	Check(
		SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&source))),
		"readback texture");
	resource->Release();
	ID3D11Device* device = nullptr;
	source->GetDevice(&device);
	ID3D11DeviceContext* context = nullptr;
	device->GetImmediateContext(&context);
	D3D11_TEXTURE2D_DESC description;
	source->GetDesc(&description);
	description.Usage          = D3D11_USAGE_STAGING;
	description.BindFlags      = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	description.MiscFlags      = 0;
	ID3D11Texture2D* staging   = nullptr;
	Check(SUCCEEDED(device->CreateTexture2D(&description, nullptr, &staging)), "create readback staging texture");
	context->CopyResource(staging, source);
	D3D11_MAPPED_SUBRESOURCE mapped;
	Check(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)), "map rendered pixels");

	std::ofstream output(path, std::ios::binary);
	output << "P6\n" << description.Width << ' ' << description.Height << "\n255\n";
	for (unsigned int y = 0; y < description.Height; ++y)
	{
		auto const* row = static_cast<unsigned char const*>(mapped.pData) + y * mapped.RowPitch;
		for (unsigned int x = 0; x < description.Width; ++x)
		{
			output.write(reinterpret_cast<char const*>(row + x * 4), 3);
		}
	}
	Check(output.good(), "write GPU frame evidence");
	context->Unmap(staging, 0);
	staging->Release();
	context->Release();
	device->Release();
	source->Release();
}

void VerifyEditor(bool capture)
{
	auto directory = std::filesystem::absolute("../Temporary/TreeScrollVerification/EditorProject");
	std::filesystem::create_directories(directory / "Settings");
	_wputenv_s(L"LOCALAPPDATA", (directory / "Settings").c_str());
	{
		std::ofstream project(directory / "project.ming");
		project << R"({"type":"ProjectSettings","version":1,"start_scene_path":"res://main.tscn"})";
		std::ofstream scene(directory / "main.tscn");
		scene << "[ming_scene format=1]\n\n[node name=\"Main\" type=\"Node\" parent=\"\"]\n";
		for (int i = 0; i < 70; ++i)
		{
			scene << "\n[node name=\"Item" << i << "\" type=\"Node\" parent=\".\"]\n";
			std::ofstream file(directory / ("item" + std::to_string(i) + ".tscn"));
			file << R"([ming_scene format=1]

[node name="Main" type="Node" parent=""]
)";
		}

	}

	MingRunConfig config;
	config.mode = MingRunMode::Editor;
	config.projectPath = directory;
	App app(config);
	app.Startup();
	Check(!app.IsQuitting(), "editor fixture startup");
	glfwHideWindow(app.GetEngine()->m_windowSystem->GetGLFWWindow());
	auto frames = [&app]()
	{
		for (int i = 0; i < 3; ++i)
		{
			app.RunFrame();
		}
	};
	frames();
	auto* ui = EditorNode::Get()->m_editorUI;
	auto* viewport = static_cast<Viewport*>(app.GetEngine()->m_sceneSystem->GetSceneTree()->GetRoot());
	InputEvent focus;
	focus.m_type = InputEventType::WindowFocus;
	focus.m_focused = true;
	viewport->ProcessInputEvent(focus);
	auto checkTree = [viewport](Tree* tree)
	{
		Check(tree != nullptr && dynamic_cast<ScrollContainer*>(tree->GetParent()) == nullptr, "editor tree has no external scroll wrapper");
		Check(tree->GetContentSize().y > tree->GetPageSize().y && tree->GetPageSize().y > 0.f, "editor tree has a bounded scrolling viewport");
		tree->SetScroll(0.f);
		Vector2 point(50.f, tree->GetRowHeight() * 1.5f);
		uint64_t expected = tree->GetItemAtPosition(point);
		viewport->SetGuiFocus(nullptr);
		Click(viewport, tree->GetGlobalPosition() + point);
		Check(expected != 0 && tree->GetSelected() == expected && Near(tree->GetScroll(), 0.f), "editor focus click preserves viewport and selects correct row");
	};
	checkTree(ui->GetFileTreeControl());
	checkTree(ui->GetSceneTreeControl());
	if (capture)
	{
		frames();
		CaptureRenderedFrame(*app.GetEngine(), "../Temporary/TreeScrollVerification/editor.ppm");
	}
	EditorUIContext context;
	context.m_rootControl = ui->GetRootControl();
	context.m_sceneTree = app.GetEngine()->m_sceneSystem->GetSceneTree();
	context.m_editorUI = ui;
	context.m_fileSystem = app.GetEngine()->m_fileSystem;
	SceneFilePopup files;
	files.Open(SceneFilePopup::Mode::Open, VirtualPath("res://"), [](VirtualPath const&) { return true; });
	files.Render(context);
	frames();
	auto findTree = [](auto&& self, Node* node) -> Tree*
	{
		if (node == nullptr)
		{
			return nullptr;
		}

		if (auto* tree = dynamic_cast<Tree*>(node))
		{
			return tree;
		}

		for (Node* child : node->GetChildren())
		{
			if (auto* tree = self(self, child))
			{
				return tree;
			}
		}

		return nullptr;
	};
	checkTree(findTree(findTree, viewport->GetGuiPopup()));
	if (capture)
	{
		frames();
		CaptureRenderedFrame(*app.GetEngine(), "../Temporary/TreeScrollVerification/files.ppm");
	}
	InputEvent escape;
	escape.m_type = InputEventType::Key;
	escape.m_keyCode = GLFW_KEY_ESCAPE;
	escape.m_pressed = true;
	viewport->ProcessInputEvent(escape);
	CreateNodePopup create;
	create.Open(ObjectID::Invalid);
	create.Render(context);
	frames();
	auto* types = findTree(findTree, viewport->GetGuiPopup());
	Check(types != nullptr && dynamic_cast<ScrollContainer*>(types->GetParent()) == nullptr, "type tree has no external scroll wrapper");
	Check(types->GetPageSize().y > 0.f, "type tree receives viewport space");
	types->SetScroll(100000.f);
	Check(types->GetScroll() <= types->GetContentSize().y - types->GetPageSize().y + 0.01f, "type tree clamps internal scrolling");
	if (capture)
	{
		frames();
		CaptureRenderedFrame(*app.GetEngine(), "../Temporary/TreeScrollVerification/types.ppm");
	}

	viewport->ProcessInputEvent(escape);
	Check(EditorNode::Get()->SetEditorScale(1.5f), "editor theme scale changes");
	frames();
	checkTree(ui->GetFileTreeControl());
	app.Shutdown();
}

}

int main(int argc, char** argv)
{
	try
	{
		if (argc > 1 && (std::string(argv[1]) == "--editor" || std::string(argv[1]) == "--editor-capture"))
		{
			VerifyEditor(std::string(argv[1]) == "--editor-capture");
			std::printf("TREE_SCROLL_EDITOR checks=%d failures=0\n", checks);
			return 0;
		}

		ClassDatabase::Startup();
		RegisterAllTypes();
		EngineConfig config;
		config.m_windowConfig.m_appName = "Ming Tree Scroll Verification";
		config.m_scriptSystemConfig.m_isEnabled = false;
		config.m_fileSystemConfig.m_isEnable = false;
		config.m_audioConfig.m_isEnable = false;
		config.m_networkConfig.m_isEnable = false;
		{
			Engine engine(config);
			Check(engine.Startup(), "engine startup");
			if (argc == 1)
			{
				glfwHideWindow(engine.m_windowSystem->GetGLFWWindow());
			}
			Verify(engine, argc > 1);
			engine.m_sceneSystem->Shutdown();
			ThemeDatabase::Shutdown();
			engine.Shutdown();
		}
		g_engine = nullptr;
		ClassDatabase::Shutdown();
		std::printf("TREE_SCROLL checks=%d failures=0\n", checks);
		return 0;
	}
	catch (std::exception const& error)
	{
		std::fprintf(stderr, "TREE_SCROLL failure: %s\n", error.what());
		return 1;
	}
}
