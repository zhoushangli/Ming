#include "MingEngine/Engine/ImGui/ImGuiSystem.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"

#include "ThirdParty/imgui/backends/imgui_impl_dx11.h"
#include "ThirdParty/imgui/backends/imgui_impl_glfw.h"
#include "ThirdParty/imgui/imgui.h"

namespace
{
char const* const kImGuiIniFilePath = "Data/.ming/imgui.ini";

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
	colors[ImGuiCol_Text]                      = RGBA(225.f, 225.f, 225.f, 1.f);
	colors[ImGuiCol_TextDisabled]              = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_WindowBg]                  = RGBA(47.f, 54.f, 65.f, 1.f);
	colors[ImGuiCol_ChildBg]                   = RGBA(47.f, 54.f, 65.f, 1.f);
	colors[ImGuiCol_PopupBg]                   = RGBA(24.f, 27.f, 32.f, 1.f);
	colors[ImGuiCol_Border]                    = RGBA(33.f, 37.f, 43.f, 1.f);
	colors[ImGuiCol_BorderShadow]              = RGBA(0.f, 0.f, 0.f, 0.f);
	colors[ImGuiCol_FrameBg]                   = RGBA(33.f, 37.f, 43.f, 1.f);
	colors[ImGuiCol_FrameBgHovered]            = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_FrameBgActive]             = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_TitleBg]                   = RGBA(24.f, 27.f, 32.f, 1.f);
	colors[ImGuiCol_TitleBgActive]             = RGBA(24.f, 27.f, 32.f, 1.f);
	colors[ImGuiCol_TitleBgCollapsed]          = RGBA(24.f, 27.f, 32.f, 1.f);
	colors[ImGuiCol_MenuBarBg]                 = RGBA(24.f, 27.f, 32.f, 1.f);
	colors[ImGuiCol_ScrollbarBg]               = RGBA(24.f, 27.f, 32.f, 1.f);
	colors[ImGuiCol_ScrollbarGrab]             = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_ScrollbarGrabHovered]      = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_ScrollbarGrabActive]       = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_CheckMark]                 = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_SliderGrab]                = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_SliderGrabActive]          = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_Button]                    = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_ButtonHovered]             = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_ButtonActive]              = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_Header]                    = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_HeaderHovered]             = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_HeaderActive]              = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_Separator]                 = RGBA(24.f, 27.f, 32.f, 1.f);
	colors[ImGuiCol_SeparatorHovered]          = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_SeparatorActive]           = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_ResizeGrip]                = RGBA(86.f, 158.f, 255.f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered]         = RGBA(86.f, 158.f, 255.f, 0.67f);
	colors[ImGuiCol_ResizeGripActive]          = RGBA(86.f, 158.f, 255.f, 0.95f);
	colors[ImGuiCol_Tab]                       = RGBA(33.f, 37.f, 43.f, 1.f);
	colors[ImGuiCol_TabHovered]                = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_TabSelected]               = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_TabSelectedOverline]       = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_TabDimmed]                 = RGBA(47.f, 54.f, 65.f, 1.f);
	colors[ImGuiCol_TabDimmedSelected]         = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_TabDimmedSelectedOverline] = RGBA(86.f, 158.f, 255.f, 0.45f);
	colors[ImGuiCol_DockingPreview]            = RGBA(86.f, 158.f, 255.f, 0.35f);
	colors[ImGuiCol_DockingEmptyBg]            = RGBA(24.f, 27.f, 32.f, 1.f);
	colors[ImGuiCol_PlotLines]                 = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_PlotLinesHovered]          = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_PlotHistogram]             = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_PlotHistogramHovered]      = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_TableHeaderBg]             = RGBA(80.f, 89.f, 104.f, 1.f);
	colors[ImGuiCol_TableBorderStrong]         = RGBA(24.f, 27.f, 32.f, 1.f);
	colors[ImGuiCol_TableBorderLight]          = RGBA(33.f, 37.f, 43.f, 0.65f);
	colors[ImGuiCol_TableRowBg]                = RGBA(0.f, 0.f, 0.f, 0.f);
	colors[ImGuiCol_TableRowBgAlt]             = RGBA(255.f, 255.f, 255.f, 0.035f);
	colors[ImGuiCol_TextSelectedBg]            = RGBA(86.f, 158.f, 255.f, 0.35f);
	colors[ImGuiCol_DragDropTarget]            = RGBA(86.f, 158.f, 255.f, 0.9f);
	colors[ImGuiCol_NavHighlight]              = RGBA(86.f, 158.f, 255.f, 1.f);
	colors[ImGuiCol_NavWindowingHighlight]     = RGBA(225.f, 225.f, 225.f, 0.7f);
	colors[ImGuiCol_NavWindowingDimBg]         = RGBA(0.f, 0.f, 0.f, 0.35f);
	colors[ImGuiCol_ModalWindowDimBg]          = RGBA(0.f, 0.f, 0.f, 0.45f);

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF("Data/Fonts/JetBrainsMono-Regular.ttf", 24.0f);
}
} // namespace

ImGuiSystem::ImGuiSystem(ImGuiConfig config) : m_config(config) {}

ImGuiSystem::~ImGuiSystem() {}

void ImGuiSystem::BindMethods() {}

void ImGuiSystem::Startup()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.IniFilename = kImGuiIniFilePath;

	ApplyGodotImGuiTheme();

	ImGui_ImplGlfw_InitForOther(g_engine->m_windowSystem->GetGLFWWindow(), false);
	g_engine->m_renderer->InitImGuiD3D11Backend();
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
}

void ImGuiSystem::EndFrame()
{
	ImGui::Render();

	g_engine->m_renderer->BindBackBuffer();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool ImGuiSystem::WantCaptureMouse() const { return ImGui::GetIO().WantCaptureMouse; }
