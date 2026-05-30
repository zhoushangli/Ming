#include "Engine/ImGUI/ImGUISystem.hpp"

#include "Engine/Core/Engine.hpp"

#include "ThirdParty/imgui/backends/imgui_impl_dx11.h"
#include "ThirdParty/imgui/backends/imgui_impl_glfw.h"
#include "ThirdParty/imgui/imgui.h"

namespace
{
ImVec4 RGBA(float r, float g, float b, float a) { return ImVec4(r / 255.f, g / 255.f, b / 255.f, a); }

void ApplyGodotImGuiTheme()
{
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha                            = 1.f;
	style.DisabledAlpha                    = 0.1f;
	style.WindowPadding                    = ImVec2(8.f, 8.f);
	style.WindowRounding                   = 10.f;
	style.WindowBorderSize                 = 0.f;
	style.WindowMinSize                    = ImVec2(30.f, 30.f);
	style.WindowTitleAlign                 = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition         = ImGuiDir_Right;
	style.ChildRounding                    = 5.f;
	style.ChildBorderSize                  = 1.f;
	style.PopupRounding                    = 10.f;
	style.PopupBorderSize                  = 0.f;
	style.FramePadding                     = ImVec2(5.f, 3.5f);
	style.FrameRounding                    = 5.f;
	style.FrameBorderSize                  = 0.f;
	style.ItemSpacing                      = ImVec2(5.f, 4.f);
	style.ItemInnerSpacing                 = ImVec2(5.f, 5.f);
	style.CellPadding                      = ImVec2(4.f, 2.f);
	style.IndentSpacing                    = 5.f;
	style.ColumnsMinSpacing                = 5.f;
	style.ScrollbarSize                    = 15.f;
	style.ScrollbarRounding                = 9.f;
	style.GrabMinSize                      = 15.f;
	style.GrabRounding                     = 5.f;
	style.TabRounding                      = 5.f;
	style.TabBorderSize                    = 0.f;
	style.TabCloseButtonMinWidthUnselected = 0.f;
	style.ColorButtonPosition              = ImGuiDir_Right;
	style.ButtonTextAlign                  = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign              = ImVec2(0.f, 0.f);

	ImVec4* colors                             = style.Colors;
	colors[ImGuiCol_Text]                      = RGBA(213.f, 218.f, 226.f, 1.f);
	colors[ImGuiCol_TextDisabled]              = RGBA(137.f, 145.f, 158.f, 1.f);
	colors[ImGuiCol_WindowBg]                  = RGBA(36.f, 44.f, 55.f, 1.f);
	colors[ImGuiCol_ChildBg]                   = RGBA(36.f, 44.f, 55.f, 1.f);
	colors[ImGuiCol_PopupBg]                   = RGBA(31.f, 38.f, 48.f, 1.f);
	colors[ImGuiCol_Border]                    = RGBA(20.f, 25.f, 32.f, 1.f);
	colors[ImGuiCol_BorderShadow]              = RGBA(0.f, 0.f, 0.f, 0.f);
	colors[ImGuiCol_FrameBg]                   = RGBA(27.f, 33.f, 42.f, 1.f);
	colors[ImGuiCol_FrameBgHovered]            = RGBA(52.f, 63.f, 78.f, 1.f);
	colors[ImGuiCol_FrameBgActive]             = RGBA(68.f, 80.f, 98.f, 1.f);
	colors[ImGuiCol_TitleBg]                   = RGBA(21.f, 26.f, 33.f, 1.f);
	colors[ImGuiCol_TitleBgActive]             = RGBA(45.f, 55.f, 68.f, 1.f);
	colors[ImGuiCol_TitleBgCollapsed]          = RGBA(21.f, 26.f, 33.f, 1.f);
	colors[ImGuiCol_MenuBarBg]                 = RGBA(21.f, 26.f, 33.f, 1.f);
	colors[ImGuiCol_ScrollbarBg]               = RGBA(31.f, 38.f, 48.f, 1.f);
	colors[ImGuiCol_ScrollbarGrab]             = RGBA(83.f, 94.f, 111.f, 1.f);
	colors[ImGuiCol_ScrollbarGrabHovered]      = RGBA(102.f, 114.f, 133.f, 1.f);
	colors[ImGuiCol_ScrollbarGrabActive]       = RGBA(117.f, 130.f, 151.f, 1.f);
	colors[ImGuiCol_CheckMark]                 = RGBA(73.f, 161.f, 255.f, 1.f);
	colors[ImGuiCol_SliderGrab]                = RGBA(67.f, 139.f, 224.f, 1.f);
	colors[ImGuiCol_SliderGrabActive]          = RGBA(93.f, 173.f, 255.f, 1.f);
	colors[ImGuiCol_Button]                    = RGBA(50.f, 60.f, 74.f, 1.f);
	colors[ImGuiCol_ButtonHovered]             = RGBA(62.f, 75.f, 92.f, 1.f);
	colors[ImGuiCol_ButtonActive]              = RGBA(78.f, 91.f, 111.f, 1.f);
	colors[ImGuiCol_Header]                    = RGBA(86.f, 97.f, 115.f, 1.f);
	colors[ImGuiCol_HeaderHovered]             = RGBA(67.f, 80.f, 99.f, 1.f);
	colors[ImGuiCol_HeaderActive]              = RGBA(74.f, 137.f, 209.f, 1.f);
	colors[ImGuiCol_Separator]                 = RGBA(20.f, 25.f, 32.f, 1.f);
	colors[ImGuiCol_SeparatorHovered]          = RGBA(73.f, 161.f, 255.f, 1.f);
	colors[ImGuiCol_SeparatorActive]           = RGBA(93.f, 173.f, 255.f, 1.f);
	colors[ImGuiCol_ResizeGrip]                = RGBA(73.f, 161.f, 255.f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered]         = RGBA(73.f, 161.f, 255.f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]          = RGBA(93.f, 173.f, 255.f, 0.95f);
	colors[ImGuiCol_Tab]                       = RGBA(35.f, 43.f, 54.f, 1.f);
	colors[ImGuiCol_TabHovered]                = RGBA(55.f, 68.f, 85.f, 1.f);
	colors[ImGuiCol_TabSelected]               = RGBA(80.f, 93.f, 112.f, 1.f);
	colors[ImGuiCol_TabSelectedOverline]       = RGBA(73.f, 161.f, 255.f, 1.f);
	colors[ImGuiCol_TabDimmed]                 = RGBA(28.f, 35.f, 44.f, 1.f);
	colors[ImGuiCol_TabDimmedSelected]         = RGBA(53.f, 64.f, 79.f, 1.f);
	colors[ImGuiCol_TabDimmedSelectedOverline] = RGBA(73.f, 161.f, 255.f, 0.45f);
	colors[ImGuiCol_DockingPreview]            = RGBA(73.f, 161.f, 255.f, 0.35f);
	colors[ImGuiCol_DockingEmptyBg]            = RGBA(31.f, 38.f, 48.f, 1.f);
	colors[ImGuiCol_PlotLines]                 = RGBA(137.f, 145.f, 158.f, 1.f);
	colors[ImGuiCol_PlotLinesHovered]          = RGBA(93.f, 173.f, 255.f, 1.f);
	colors[ImGuiCol_PlotHistogram]             = RGBA(67.f, 139.f, 224.f, 1.f);
	colors[ImGuiCol_PlotHistogramHovered]      = RGBA(93.f, 173.f, 255.f, 1.f);
	colors[ImGuiCol_TableHeaderBg]             = RGBA(45.f, 55.f, 68.f, 1.f);
	colors[ImGuiCol_TableBorderStrong]         = RGBA(20.f, 25.f, 32.f, 1.f);
	colors[ImGuiCol_TableBorderLight]          = RGBA(20.f, 25.f, 32.f, 0.65f);
	colors[ImGuiCol_TableRowBg]                = RGBA(0.f, 0.f, 0.f, 0.f);
	colors[ImGuiCol_TableRowBgAlt]             = RGBA(255.f, 255.f, 255.f, 0.035f);
	colors[ImGuiCol_TextSelectedBg]            = RGBA(73.f, 161.f, 255.f, 0.35f);
	colors[ImGuiCol_DragDropTarget]            = RGBA(73.f, 161.f, 255.f, 0.9f);
	colors[ImGuiCol_NavHighlight]              = RGBA(73.f, 161.f, 255.f, 1.f);
	colors[ImGuiCol_NavWindowingHighlight]     = RGBA(213.f, 218.f, 226.f, 0.7f);
	colors[ImGuiCol_NavWindowingDimBg]         = RGBA(0.f, 0.f, 0.f, 0.35f);
	colors[ImGuiCol_ModalWindowDimBg]          = RGBA(0.f, 0.f, 0.f, 0.45f);

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("Data/Fonts/JetBrainsMono-Regular.ttf", 24.0f);
}
} // namespace

ImGuiSystem::ImGuiSystem(ImGuiConfig config) : m_config(config) {}

ImGuiSystem::~ImGuiSystem() {}

void ImGuiSystem::Startup()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	ApplyGodotImGuiTheme();

	ImGui_ImplGlfw_InitForOther(g_engine->m_window->GetGLFWWindow(), true);
	ImGui_ImplDX11_Init(g_engine->m_renderer->GetD3DDevice(), g_engine->m_renderer->GetD3DDeviceContext());
}

void ImGuiSystem::Shutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiSystem::BeginFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	RenderEditorUI();
}

void ImGuiSystem::EndFrame()
{
	ImGui::Render();

	g_engine->m_renderer->BeginUIPass();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiSystem::RenderEditorUI()
{
	RenderMainMenuBar();
	RenderDockSpace();

	if (m_showDemoWindow)
	{
		ImGui::ShowDemoWindow(&m_showDemoWindow);
	}
	if (m_showScenePanel)
	{
		RenderScenePanel();
	}
	if (m_showFileSystemPanel)
	{
		RenderFileSystemPanel();
	}
	if (m_showViewportPanel)
	{
		RenderViewportPanel();
	}
	if (m_showInspectorPanel)
	{
		RenderInspectorPanel();
	}
	if (m_showOutputPanel)
	{
		RenderOutputPanel();
	}
}

void ImGuiSystem::RenderMainMenuBar()
{
	if (!ImGui::BeginMainMenuBar())
	{
		return;
	}

	if (ImGui::BeginMenu("Scene"))
	{
		ImGui::MenuItem("New Scene");
		ImGui::MenuItem("Open Scene...");
		ImGui::Separator();
		ImGui::MenuItem("Save Scene");
		ImGui::MenuItem("Save Scene As...");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Project"))
	{
		ImGui::MenuItem("Project Settings...");
		ImGui::MenuItem("Reload Project");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Debug"))
	{
		ImGui::MenuItem("Start");
		ImGui::MenuItem("Pause");
		ImGui::MenuItem("Stop");
		ImGui::MenuItem("Step Frame");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Editor"))
	{
		ImGui::MenuItem("Editor Settings...");
		ImGui::MenuItem("Reset Layout");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window"))
	{
		ImGui::MenuItem("Scene", nullptr, &m_showScenePanel);
		ImGui::MenuItem("FileSystem", nullptr, &m_showFileSystemPanel);
		ImGui::MenuItem("Viewport", nullptr, &m_showViewportPanel);
		ImGui::MenuItem("Inspector", nullptr, &m_showInspectorPanel);
		ImGui::MenuItem("Output", nullptr, &m_showOutputPanel);
		ImGui::Separator();
		ImGui::MenuItem("Dear ImGui Demo", nullptr, &m_showDemoWindow);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Help"))
	{
		ImGui::MenuItem("About Project QingChen");
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void ImGuiSystem::RenderDockSpace()
{
	ImGuiViewport const* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
	windowFlags |= ImGuiWindowFlags_NoTitleBar;
	windowFlags |= ImGuiWindowFlags_NoCollapse;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	windowFlags |= ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::Begin("QingChen Editor DockSpace", nullptr, windowFlags);
	ImGui::PopStyleVar(2);

	ImGuiID dockspaceId = ImGui::GetID("QingChenEditorDockSpace");
	ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None);

	ImGui::End();
}

void ImGuiSystem::RenderScenePanel()
{
	ImGui::Begin("Scene", &m_showScenePanel);

	static char filterNodes[64] = "";
	ImGui::InputTextWithHint("##FilterNodes", "Filter Nodes", filterNodes, sizeof(filterNodes));
	ImGui::Separator();

	if (ImGui::TreeNodeEx("WorldRoot", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TreeNodeEx("Player", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		ImGui::TreeNodeEx("DirectionalLight", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		if (ImGui::TreeNodeEx("MeshRoot", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TreeNodeEx("Cube_01", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TreeNodeEx("Cube_02", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}

	ImGui::End();
}

void ImGuiSystem::RenderFileSystemPanel()
{
	ImGui::Begin("FileSystem", &m_showFileSystemPanel);

	ImGui::TextUnformatted("res://");
	ImGui::Separator();
	static char filterFiles[64] = "";
	ImGui::InputTextWithHint("##FilterFiles", "Filter Files", filterFiles, sizeof(filterFiles));

	if (ImGui::TreeNodeEx("Data", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TreeNodeEx("Images", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		if (ImGui::TreeNodeEx("Models", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TreeNodeEx("Cube.fbx", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TreeNodeEx("Character.fbx", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TreePop();
		}
		ImGui::TreeNodeEx("Shaders", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		ImGui::TreeNodeEx("Audio", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		ImGui::TreePop();
	}

	ImGui::End();
}

void ImGuiSystem::RenderViewportPanel()
{
	ImGui::Begin("Viewport", &m_showViewportPanel);

	ImGui::Button("Select");
	ImGui::SameLine();
	ImGui::Button("Move");
	ImGui::SameLine();
	ImGui::Button("Rotate");
	ImGui::SameLine();
	ImGui::Button("Scale");
	ImGui::SameLine();
	static bool snap = false;
	ImGui::Checkbox("Snap", &snap);
	ImGui::Separator();

	ImVec2 const availableSize = ImGui::GetContentRegionAvail();
	ImVec2 const min           = ImGui::GetCursorScreenPos();
	ImVec2 const max           = ImVec2(min.x + availableSize.x, min.y + availableSize.y);
	ImDrawList*  drawList      = ImGui::GetWindowDrawList();

	drawList->AddRectFilled(min, max, IM_COL32(30, 34, 40, 255));
	drawList->AddRect(min, max, IM_COL32(85, 95, 110, 255));
	drawList->AddText(ImVec2(min.x + 16.f, min.y + 16.f), IM_COL32(190, 200, 210, 255), "Scene Viewport Placeholder");
	drawList->AddText(
		ImVec2(min.x + 16.f, min.y + 38.f),
		IM_COL32(140, 150, 160, 255),
		"Next stage: render scene texture here with ImGui::Image()."
	);

	ImGui::Dummy(availableSize);

	ImGui::End();
}

void ImGuiSystem::RenderInspectorPanel()
{
	ImGui::Begin("Inspector", &m_showInspectorPanel);

	ImGui::TextUnformatted("Selected: Player");
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static float position[3] = {0.f, 0.f, 0.f};
		static float rotation[3] = {0.f, 0.f, 0.f};
		static float scale[3]    = {1.f, 1.f, 1.f};

		ImGui::DragFloat3("Position", position, 0.1f);
		ImGui::DragFloat3("Rotation", rotation, 0.1f);
		ImGui::DragFloat3("Scale", scale, 0.1f);
	}

	if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static bool visible = true;
		ImGui::Checkbox("Visible", &visible);
		ImGui::TextUnformatted("Material: Default");
		ImGui::TextUnformatted("Mesh: Cube");
	}

	ImGui::End();
}

void ImGuiSystem::RenderOutputPanel()
{
	ImGui::Begin("Output", &m_showOutputPanel);

	if (ImGui::BeginTabBar("OutputTabs"))
	{
		if (ImGui::BeginTabItem("Output"))
		{
			ImGui::TextUnformatted("[Info] Editor started");
			ImGui::TextUnformatted("[Info] Loaded scene: Untitled");
			ImGui::TextUnformatted("[Warning] Viewport is a placeholder in this phase");
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Console"))
		{
			ImGui::TextUnformatted("> Console input placeholder");
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Profiler"))
		{
			ImGui::TextUnformatted("Frame: -- ms");
			ImGui::TextUnformatted("Draw Calls: --");
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
}
