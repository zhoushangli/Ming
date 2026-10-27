#include "MingEngine/Engine/Application/App.hpp"

#include "MingEngine/Core/Clock.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Memory.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Render/Color.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/JetBrainsMonoFont.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Engine/ImGui/ImGuiSystem.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Script/CSharpScript.hpp"
#include "MingEngine/Engine/Script/CSharpScriptGenerator.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/Container.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"

#include "ThirdParty/GLFW/glfw3.h"
#include "ThirdParty/imgui/imgui.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <commdlg.h>
#include <memory>
#include <string>

#pragma comment(lib, "Comdlg32.lib")

App* g_app = nullptr;

App::App(MingRunConfig const& config) : m_runConfig(config)
{
	EngineConfig engineConfig;
	engineConfig.m_imguiConfig.m_isEnable = false;
	engineConfig.m_windowConfig.m_clientAspect     = 16.f / 9.f;
	engineConfig.m_windowConfig.m_appName          = "MingEngine";
	engineConfig.m_fileSystemConfig.m_resourceRoot = config.projectPath;
	if (config.mode == MingRunMode::ProjectManager)
	{
		engineConfig.m_windowConfig.m_appName = "MingEngine - Project Manager";
	}

	g_engine = new Engine(engineConfig);
	g_engine->SetEditorMode(config.mode == MingRunMode::Editor);
}

App::~App()
{
	ShutdownScene();

	delete g_engine;
	g_engine = nullptr;
}

void App::Startup()
{
	ClassDatabase::Startup();

	RegisterAllTypes();

	g_engine->Startup();

	if (m_runConfig.mode == MingRunMode::Editor)
	{
        if (g_engine->m_imguiSystem != nullptr)
        {
            EditorIcons::Startup();
        }
		StartupEditor();
	}
	else if (m_runConfig.mode == MingRunMode::Game)
	{
		StartupGame();
	}
	else
	{
		GLFWwindow* window = g_engine->m_windowSystem->GetGLFWWindow();
		glfwRestoreWindow(window);
		glfwSetWindowSize(window, 720, 360);
	}

	m_quitEventID = RegisterEvent("Quit", App::OnQuit);
}

void App::Shutdown()
{
	ShutdownScene();

	if (g_engine != nullptr && g_engine->m_eventSystem != nullptr)
	{
		UnregisterEvent("Quit", m_quitEventID);
		m_quitEventID = 0;
	}

	ResourceLoader::Shutdown();

	EditorIcons::Shutdown();

	g_engine->Shutdown();

	ClassDatabase::Shutdown();
}

void App::RunMainLoop()
{
	while (!IsQuitting())
	{
		RunFrame();
	}
}

void App::Update(float deltaSeconds)
{
	if (m_runConfig.mode == MingRunMode::ProjectManager)
	{
		DrawProjectManager();
        m_sceneTree->UpdateScene(deltaSeconds);
		return;
	}

	GLFWwindow* window   = g_engine->m_windowSystem->GetGLFWWindow();
	bool const  hasFocus = window != nullptr && glfwGetWindowAttrib(window, GLFW_FOCUSED);

	if (!hasFocus)
	{
		g_engine->m_inputSystem->SetCursorMode(CursorMode::POINTER);
		if (!hasFocus)
		{
			g_engine->m_inputSystem->ClearCursorDelta();
		}
	}

	float const sceneDeltaSeconds = m_clock != nullptr ? static_cast<float>(m_clock->GetDeltaSeconds()) : deltaSeconds;
	if (m_sceneTree != nullptr)
	{
		if (m_uiResizeSizeLabel != nullptr)
		{
			IntVec2 const size = static_cast<Viewport*>(m_sceneTree->GetRoot())->GetOutputResolution();
			m_uiResizeSizeLabel->SetText(Stringf("Viewport: %d x %d", size.x, size.y).c_str());
		}

		m_sceneTree->UpdateScene(sceneDeltaSeconds);
	}
}

void App::Render() const
{
	if (g_engine != nullptr && g_engine->m_renderServer != nullptr)
	{
		g_engine->m_renderServer->Render();
	}
}

void App::BeginFrame()
{
	g_engine->BeginFrame();
}

void App::EndFrame()
{
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->FlushPendingNode();
	}

	g_engine->EndFrame();

	if (m_shouldRestart)
	{
		RestartImmediately();
		m_shouldRestart = false;
	}
}

void App::RunFrame()
{
	BeginFrame();

	Clock::TickSystemClock();
	float const deltaSeconds = static_cast<float>(Clock::GetSystemClock().GetDeltaSeconds());

	Update(deltaSeconds);
	Render();
	EndFrame();
}

void App::Restart()
{
	m_shouldRestart = true;
}

void App::Quit()
{
	if (EditorNode* editor = EditorNode::Get(); editor != nullptr && g_engine->m_imguiSystem == nullptr)
	{
		editor->m_editorUI->FinishNativeEditing();
	}

	if (EditorNode* editor = EditorNode::Get();
		editor != nullptr && g_engine->m_imguiSystem == nullptr && editor->IsSceneDirty())
	{
		glfwSetWindowShouldClose(g_engine->m_windowSystem->GetGLFWWindow(), GLFW_FALSE);
		editor->RequestQuit();
		return;
	}

	m_shouldQuit = true;
}

bool App::LaunchGame()
{
	// 1) Get the current executable path
	std::wstring executablePath(32768, L'\0');

	DWORD const length = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));

	if (length == 0 || length >= executablePath.size())
	{
		ERR_PRINT("Failed to get the executable path.\n");
		return false;
	}

	executablePath.resize(length);

	// 2) Resolve the current project directory
	if (m_runConfig.projectPath.empty())
	{
		ERR_PRINT("Cannot launch game: project path is empty.\n");
		return false;
	}

	std::error_code             error;
	std::filesystem::path const projectDirectory = std::filesystem::absolute(m_runConfig.projectPath, error);

	if (error)
	{
		ERR_PRINT("Failed to resolve the project directory.\n");
		return false;
	}

	std::wstring const projectArgument = (projectDirectory / L".").wstring();

	// 3) Build the game-mode command line
	std::wstring commandLine = L"\"" + executablePath + L"\" --game --project \"" + projectArgument + L"\"";

	STARTUPINFOW startupInfo{};
	startupInfo.cb = sizeof(startupInfo);

	PROCESS_INFORMATION processInfo{};

	// 4) Start the game process
	if (!CreateProcessW(
			executablePath.c_str(),
			commandLine.data(),
			nullptr,
			nullptr,
			FALSE,
			0,
			nullptr,
			nullptr,
			&startupInfo,
			&processInfo))
	{
		DWORD const       errorCode = GetLastError();
		std::string const message   = "Failed to launch game. Windows error: " + std::to_string(errorCode) + "\n";

		ERR_PRINT(message.c_str());
		return false;
	}

	// 5) Release the launch-only process handles
	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
	return true;
}

bool App::OnQuit([[maybe_unused]] EventArgs& args)
{
	if (g_app != nullptr)
	{
		g_app->Quit();
	}

	return true;
}

void App::RestartImmediately()
{
	ShutdownScene();

	if (m_runConfig.mode == MingRunMode::Editor)
	{
		StartupEditor();
	}
	else if (m_runConfig.mode == MingRunMode::Game)
	{
		StartupGame();
	}
}

void App::DrawProjectManager()
{
	if (g_engine->m_imguiSystem == nullptr)
	{
		if (m_sceneTree == nullptr)
		{
			m_sceneTree      = MemNew<SceneTree>();
			auto  font       = JetBrainsMonoFont::CreateNativeFont();
			auto* background = MemNew<ColorRect>();
			background->SetName("ProjectManager");
			background->SetColor(Color(25, 29, 37));
			background->SetClipContents(true);
			m_sceneTree->GetRoot()->AddNode(background);
			auto* content = MemNew<VBoxContainer>();
			content->SetPosition(Vector2(28.f, 28.f));
			content->SetSeparation(20.f);
			background->AddNode(content);
			auto* title = MemNew<Label>();
			title->SetText("MingEngine - Project Manager");
			title->SetFontSize(24);
			content->AddNode(title);
			auto* hint = MemNew<Label>();
			hint->SetText("Select project.ming to open a project.");
			content->AddNode(hint);
			auto* open = MemNew<Button>();
			open->SetText("Open Project...");
			open->SetPressedCallback(
				[this]()
				{
					OpenProject();
				});
			content->AddNode(open);
			m_projectManagerErrorLabel = MemNew<Label>();
			content->AddNode(m_projectManagerErrorLabel);
			JetBrainsMonoFont::ApplyNativeFont(background, font);
		}

		auto*         background = static_cast<Control*>(m_sceneTree->GetRoot()->GetChildren()[0]);
		Vector2 const size(static_cast<Viewport*>(m_sceneTree->GetRoot())->GetOutputResolution());
		background->UpdateSize(size);
		static_cast<Control*>(background->GetChildren()[0])
			->UpdateSize(Vector2(std::max(1.f, size.x - 56.f), std::max(1.f, size.y - 56.f)));
		m_projectManagerErrorLabel->SetText(m_projectManagerError);
		return;
	}

	ImGuiViewport const* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.f, 24.f));
	if (ImGui::Begin(
			"Project Manager",
			nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::TextUnformatted("MingEngine");
		ImGui::Spacing();
		ImGui::TextWrapped("Select project.ming to open a project.");
		ImGui::Spacing();
		if (ImGui::Button("Open Project...", ImVec2(240.f, 42.f)))
		{
			OpenProject();
		}

		if (!m_projectManagerError.empty())
		{
			ImGui::Spacing();
			ImGui::TextWrapped("%s", m_projectManagerError.c_str());
		}
	}

	ImGui::End();
	ImGui::PopStyleVar();
}

void App::OpenProject()
{
	m_projectManagerError.clear();

	// 1) Select an existing project settings file without changing the working directory
	std::wstring  selectedPath(32768, L'\0');
	OPENFILENAMEW dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.hwndOwner   = static_cast<HWND>(g_engine->m_windowSystem->GetHwnd());
	dialog.lpstrFilter = L"Ming project (project.ming)\0*.ming\0\0";
	dialog.lpstrFile   = selectedPath.data();
	dialog.nMaxFile    = static_cast<DWORD>(selectedPath.size());
	dialog.lpstrTitle  = L"Open Ming Project";
	dialog.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
	if (!GetOpenFileNameW(&dialog))
	{
		DWORD const error = CommDlgExtendedError();
		if (error != 0)
		{
			m_projectManagerError = "Failed to open the file dialog. Windows error: " + std::to_string(error);
		}

		return;
	}

	// 2) Validate the selected file before launching the editor
	std::filesystem::path const settingsPath(selectedPath.c_str());
	std::error_code             error;
	if (settingsPath.filename() != L"project.ming" || !std::filesystem::is_regular_file(settingsPath, error) || error)
	{
		m_projectManagerError = "Please select an existing project.ming file.";
		return;
	}

	std::wstring executablePath(32768, L'\0');
	DWORD const  length = GetModuleFileNameW(nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));
	if (length == 0 || length >= executablePath.size())
	{
		m_projectManagerError = "Failed to get the executable path.";
		return;
	}

	executablePath.resize(length);

	// 3) Launch the editor with an absolute project path and close the manager only on success
	std::wstring const projectArgument = (settingsPath.parent_path() / L".").wstring();
	std::wstring       commandLine     = L"\"" + executablePath + L"\" --editor --project \"" + projectArgument + L"\"";
	std::wstring const workingDirectory = std::filesystem::path(executablePath).parent_path().wstring();
	STARTUPINFOW       startupInfo{};
	startupInfo.cb = sizeof(startupInfo);
	PROCESS_INFORMATION processInfo{};
	if (!CreateProcessW(
			executablePath.c_str(),
			commandLine.data(),
			nullptr,
			nullptr,
			FALSE,
			0,
			nullptr,
			workingDirectory.c_str(),
			&startupInfo,
			&processInfo))
	{
		m_projectManagerError = "Failed to launch the editor. Windows error: " + std::to_string(GetLastError());
		return;
	}

	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
	Quit();
}

void App::StartupEditor()
{
	glfwSetWindowSizeLimits(g_engine->m_windowSystem->GetGLFWWindow(), 900, 540, GLFW_DONT_CARE, GLFW_DONT_CARE);
	m_clock     = new Clock();
	m_sceneTree = MemNew<SceneTree>();

	auto editorNode = new EditorNode();
	editorNode->SetName("EditorNode");
	m_sceneTree->GetRoot()->AddNode(editorNode);
	VirtualPath const& startScenePath = ProjectSettings::Get()->m_startScenePath;
	if (startScenePath.IsValid())
	{
		editorNode->LoadScene(startScenePath);
	}
}

void App::StartupGame()
{
	m_clock     = new Clock();
	m_sceneTree = MemNew<SceneTree>();

	// 1) Load the font and build the visual test helpers
	Ref<FontResource> font = ResourceLoader::Load("res://fusion-pixel-12px-proportional-zh_hans.ttf");
	if (!font.IsValid())
	{
		font = JetBrainsMonoFont::CreateNativeFont();
		WARN_PRINT("UI layout test is using the embedded fallback font.");
	}

	auto addRect = [](Node* parent, char const* name, Color const& color, Vector2 const& minimum)
	{
		ColorRect* rect = MemNew<ColorRect>();
		rect->SetName(name);
		rect->SetColor(color);
		rect->SetCustomMinimumSize(minimum);
		parent->AddNode(rect);
		return rect;
	};

	auto addLabel = [&font](Node* parent, char const* name, char const* text, int fontSize)
	{
		Label* label = MemNew<Label>();
		label->SetName(name);
		label->SetFont(font);
		label->SetFontSize(fontSize);
		label->SetText(text);
		parent->AddNode(label);
		return label;
	};

	// 2) Build three nested margins that follow the root Viewport in both axes
	MarginContainer* example = MemNew<MarginContainer>();
	example->SetName("UIResizeTest");
	example->SetMargins(24.0f, 24.0f, 24.0f, 24.0f);
	addRect(example, "OuterBackground", Color(40, 80, 125), Vector2::Zero);

	MarginContainer* nested = MemNew<MarginContainer>();
	nested->SetName("MiddleMargin");
	nested->SetMargins(16.0f, 16.0f, 16.0f, 16.0f);
	example->AddNode(nested);
	addRect(nested, "MiddleBackground", Color(35, 125, 110), Vector2::Zero);

	MarginContainer* content = MemNew<MarginContainer>();
	content->SetName("InnerMargin");
	content->SetMargins(16.0f, 16.0f, 16.0f, 16.0f);
	nested->AddNode(content);
	addRect(content, "InnerBackground", Color(25, 30, 40), Vector2::Zero);

	VBoxContainer* column = MemNew<VBoxContainer>();
	column->SetName("Content");
	column->SetSeparation(12.0f);
	content->AddNode(column);

	// 3) Build the layout comparison header
	Label* title = addLabel(column, "Title", "Ming UI Size Flags Test", 28);
	title->SetHorizontalAlignment(HorizontalAlignment::Center);
	Label* hint = addLabel(column, "Hint", "Resize window. Dark areas show allocated rectangles.", 20);
	hint->SetHorizontalAlignment(HorizontalAlignment::Center);
	m_uiResizeSizeLabel = addLabel(column, "ViewportSize", "Viewport", 20);
	m_uiResizeSizeLabel->SetHorizontalAlignment(HorizontalAlignment::Center);

	// 4) Verify button activation, disabling, and independent toggles
	HBoxContainer* buttons = MemNew<HBoxContainer>();
	buttons->SetName("ButtonInteractionTest");
	buttons->SetSeparation(12.f);
	column->AddNode(buttons);

	auto addButton = [&font, buttons](char const* name, bool toggle)
	{
		Button* button = MemNew<Button>();
		button->SetName(name);
		button->SetFont(font);
		button->SetFontSize(20);
		button->SetText(name);
		button->SetToggleMode(toggle);
		buttons->AddNode(button);
		return button;
	};

	Button*        regenerate   = addButton("Regenerate", false);
	Button*        disable      = addButton("Disable Regenerate", true);
	Button*        modeA        = addButton("Mode A", true);
	Button*        modeB        = addButton("Mode B", true);
	Label*         status       = addLabel(column, "ButtonStatus", "Regenerate: 0 | Mode A: off | Mode B: off", 18);
	ObjectID const statusID     = status->GetObjectID();
	ObjectID const regenerateID = regenerate->GetObjectID();
	ObjectID const modeAID      = modeA->GetObjectID();
	ObjectID const modeBID      = modeB->GetObjectID();
	auto           updateStatus = [this, statusID, modeAID, modeBID]()
	{
		Label*  label = ObjectDatabase::GetInstance<Label>(statusID);
		Button* a     = ObjectDatabase::GetInstance<Button>(modeAID);
		Button* b     = ObjectDatabase::GetInstance<Button>(modeBID);
		if (label != nullptr && a != nullptr && b != nullptr)
		{
			label->SetText(Stringf(
							   "Regenerate: %d | Mode A: %s | Mode B: %s",
							   m_uiRegenerateCount,
							   a->IsPressed() ? "on" : "off",
							   b->IsPressed() ? "on" : "off")
							   .c_str());
		}
	};

	regenerate->SetPressedCallback(
		[this, updateStatus]()
		{
			++m_uiRegenerateCount;
			updateStatus();
		});
	disable->SetToggledCallback(
		[regenerateID](bool selected)
		{
			if (Button* button = ObjectDatabase::GetInstance<Button>(regenerateID))
			{
				button->SetDisabled(selected);
			}
		});
	modeA->SetToggledCallback(
		[updateStatus](bool)
		{
			updateStatus();
		});
	modeB->SetToggledCallback(
		[updateStatus](bool)
		{
			updateStatus();
		});

	// 5) Verify single-line editing, selection, scrolling, and state changes
	addLabel(column, "LineEditHint", "LineEdit: Enter applies, Esc restores, Tab/click applies nonempty text", 16);
	HBoxContainer* editRow = MemNew<HBoxContainer>();
	editRow->SetName("LineEditInteractionTest");
	editRow->SetSeparation(12.f);
	column->AddNode(editRow);

	LineEdit* firstEdit = MemNew<LineEdit>();
	firstEdit->SetName("NameInput");
	firstEdit->SetFont(font);
	firstEdit->SetFontSize(20);
	firstEdit->SetPlaceholder(U"Enter a name / \u8f93\u5165\u540d\u79f0");
	firstEdit->SetText("Player");
	firstEdit->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	editRow->AddNode(firstEdit);

	String const longName   = U"Long name / \u957f\u540d\u79f0: ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 -- "
							  U"ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 -- ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 -- "
							  U"ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 -- scroll to the end";
	LineEdit*    secondEdit = MemNew<LineEdit>();
	secondEdit->SetName("LongNameInput");
	secondEdit->SetFont(font);
	secondEdit->SetFontSize(20);
	secondEdit->SetText(longName);
	secondEdit->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	editRow->AddNode(secondEdit);

	// 1) Keep each field's snapshot and committed values outside LineEdit
	struct EditExample
	{
		ObjectID    m_editID;
		String      m_values[2];
		String      m_original;
		int         m_target      = 0;
		int         m_starts      = 0;
		int         m_ends        = 0;
		int         m_writes      = 0;
		char const* m_result      = "Ready";
		bool        m_focusOnEnd  = false;
		bool        m_removeOnEnd = false;
	};

	auto bindEdit = [&addLabel, column](LineEdit* edit, LineEdit* other, char const* name)
	{
		auto state              = std::make_shared<EditExample>();
		state->m_editID         = edit->GetObjectID();
		state->m_values[0]      = edit->GetText();
		state->m_values[1]      = "Enemy";
		state->m_original       = edit->GetText();
		ObjectID const statusID = addLabel(column, name, "", 14)->GetObjectID();
		ObjectID const otherID  = other->GetObjectID();
		auto           update   = [state, statusID, name]()
		{
			if (Label* label = ObjectDatabase::GetInstance<Label>(statusID))
			{
				LineEdit*    field = ObjectDatabase::GetInstance<LineEdit>(state->m_editID);
				String const draft = field != nullptr ? field->GetText() : String("<removed>");
				label->SetText(Stringf(
								   "%s[%d] before:%s draft:%s saved:%s | %s | start:%d end:%d writes:%d",
								   name,
								   state->m_target,
								   state->m_original.Substr(0, 12).ToUtf8().c_str(),
								   draft.Substr(0, 12).ToUtf8().c_str(),
								   state->m_values[state->m_target].Substr(0, 12).ToUtf8().c_str(),
								   state->m_result,
								   state->m_starts,
								   state->m_ends,
								   state->m_writes)
								   .c_str());
			}
		};
		edit->SetEditingStartedCallback(
			[state, update](String const& text)
			{
				state->m_original = text;
				++state->m_starts;
				state->m_result = "Editing";
				update();
			});
		edit->SetTextChangedCallback(
			[update](String const&)
			{
				update();
			});
		edit->SetEditingEndedCallback(
			[state, update, otherID](String const& text, LineEdit::EditEndReason reason)
			{
				++state->m_ends;
				bool const apply =
					reason == LineEdit::EditEndReason::Submitted || reason == LineEdit::EditEndReason::FocusLost;
				if (apply && !text.IsEmpty())
				{
					if (state->m_values[state->m_target] != text)
					{
						state->m_values[state->m_target] = text;
						++state->m_writes;
					}

					state->m_result = reason == LineEdit::EditEndReason::Submitted ? "Submitted" : "FocusLost";
				}
				else
				{
					state->m_result = apply                                          ? "Invalid:empty"
									  : reason == LineEdit::EditEndReason::Cancelled ? "Cancelled"
																					 : "Interrupted";
					if (LineEdit* field = ObjectDatabase::GetInstance<LineEdit>(state->m_editID))
					{
						field->SetText(state->m_original);
					}
				}

				// 2) Exercise callbacks that change focus or remove their own field
				if (state->m_focusOnEnd)
				{
					if (LineEdit* next = ObjectDatabase::GetInstance<LineEdit>(otherID))
					{
						next->GrabFocus();
					}
				}

				if (state->m_removeOnEnd)
				{
					if (LineEdit* field = ObjectDatabase::GetInstance<LineEdit>(state->m_editID))
					{
						field->QueueFree();
					}

					state->m_editID = ObjectID::Invalid;
				}

				update();
			});
		update();
		return std::make_pair(state, update);
	};

	auto           firstExample  = bindEdit(firstEdit, secondEdit, "First");
	auto           secondExample = bindEdit(secondEdit, firstEdit, "Second");
	ObjectID const editRowID     = editRow->GetObjectID();

	// 3) Interrupt fields directly without moving focus to a test button first
	char const* const editButtonNames[] = { "Read only",     "Disabled",     "Hide fields",  "Reset text",
											"Switch object", "Remove first", "Focus on end", "Remove on end" };
	HBoxContainer*    editButtons       = nullptr;
	for (int index = 0; index < 8; ++index)
	{
		if (index % 4 == 0)
		{
			editButtons = MemNew<HBoxContainer>();
			editButtons->SetName(index == 0 ? "LineEditStateTest" : "LineEditEndTest");
			editButtons->SetSeparation(12.f);
			column->AddNode(editButtons);
		}

		Button* button = MemNew<Button>();
		button->SetFont(font);
		button->SetFontSize(16);
		button->SetFocusMode(Control::FocusMode::None);
		button->SetText(editButtonNames[index]);
		button->SetToggleMode(index < 3 || index >= 6);
		editButtons->AddNode(button);
		button->SetPressedCallback(
			[firstExample, secondExample, editRowID, longName, index, buttonID = button->GetObjectID()]()
			{
				auto       state    = firstExample.first;
				LineEdit*  field    = ObjectDatabase::GetInstance<LineEdit>(state->m_editID);
				Button*    source   = ObjectDatabase::GetInstance<Button>(buttonID);
				bool const selected = source != nullptr && source->IsPressed();
				if (index == 0 && field != nullptr)
				{
					field->SetEditable(!selected);
				}
				else if (index == 1 && field != nullptr)
				{
					field->SetDisabled(selected);
				}
				else if (index == 2)
				{
					if (Control* row = ObjectDatabase::GetInstance<Control>(editRowID))
					{
						row->SetVisible(!selected);
					}
				}
				else if (index == 3 || index == 4)
				{
					if (field != nullptr)
					{
						field->EndEditing();
						field = ObjectDatabase::GetInstance<LineEdit>(state->m_editID);
						if (field != nullptr && !field->IsQueuedForDeletion())
						{
							if (index == 4)
							{
								state->m_target = 1 - state->m_target;
							}
							else
							{
								state->m_values[state->m_target] = "Player";
							}

							state->m_original = state->m_values[state->m_target];
							field->SetText(state->m_original);
						}
					}

					if (index == 3)
					{
						auto second = secondExample.first;
						if (LineEdit* other = ObjectDatabase::GetInstance<LineEdit>(second->m_editID))
						{
							other->EndEditing();
							second->m_values[second->m_target] = longName;
							second->m_original                 = longName;
							other = ObjectDatabase::GetInstance<LineEdit>(second->m_editID);
							if (other != nullptr)
							{
								other->SetText(longName);
							}
						}
					}
				}
				else if (index == 5 && field != nullptr)
				{
					field->EndEditing();
					if (LineEdit* remaining = ObjectDatabase::GetInstance<LineEdit>(state->m_editID))
					{
						remaining->QueueFree();
					}

					state->m_editID = ObjectID::Invalid;
				}
				else if (index == 6)
				{
					state->m_focusOnEnd = selected;
				}
				else if (index == 7)
				{
					state->m_removeOnEnd = selected;
				}

				firstExample.second();
				secondExample.second();
			});
	}

	// 6) Compare fill and shrink alignment inside visible allocated rectangles
	auto addAlignmentRow = [&](char const* name, bool vertical)
	{
		HBoxContainer* row = MemNew<HBoxContainer>();
		row->SetName(name);
		row->SetSeparation(12.0f);
		if (vertical)
		{
			row->SetVerticalSizeFlags(Control::Size_ExpandFill);
		}

		column->AddNode(row);

		char const* const names[] = { "Fill", "Shrink Begin", "Shrink Center", "Shrink End" };
		int const         flags[] = { Control::Size_Fill,
									  Control::Size_ShrinkBegin,
									  Control::Size_ShrinkCenter,
									  Control::Size_ShrinkEnd };
		Color const colors[] = { Color(70, 145, 235), Color(220, 80, 80), Color(80, 190, 120), Color(220, 180, 70) };
		for (int index = 0; index < 4; ++index)
		{
			VBoxContainer* group = MemNew<VBoxContainer>();
			group->SetName(names[index]);
			group->SetSeparation(4.0f);
			group->SetHorizontalSizeFlags(Control::Size_ExpandFill);
			row->AddNode(group);
			addLabel(group, "Caption", names[index], 16);

			MarginContainer* slot = MemNew<MarginContainer>();
			slot->SetName("AllocatedArea");
			slot->SetCustomMinimumSize(Vector2(96.0f, vertical ? 80.0f : 36.0f));
			slot->SetVerticalSizeFlags(Control::Size_ExpandFill);
			group->AddNode(slot);
			addRect(slot, "Background", Color(45, 55, 70), Vector2::Zero);

			ColorRect* sample = addRect(slot, "Sample", colors[index], Vector2(48.0f, 24.0f));
			sample->SetHorizontalSizeFlags(vertical ? Control::Size_ShrinkCenter : flags[index]);
			sample->SetVerticalSizeFlags(vertical ? flags[index] : Control::Size_ShrinkCenter);
		}
	};

	addLabel(column, "HorizontalHint", "Horizontal alignment", 18);
	addAlignmentRow("HorizontalAlignment", false);
	addLabel(column, "VerticalHint", "Vertical alignment (this row expands vertically)", 18);
	addAlignmentRow("VerticalAlignment", true);

	// 7) Compare fixed width, expand with fill, and expand with shrink center
	addLabel(column, "ExpandHint", "HBox: fixed red | Expand + Fill green | Expand + Center yellow", 18);
	MarginContainer* expandArea = MemNew<MarginContainer>();
	expandArea->SetName("ExpandArea");
	expandArea->SetMargins(8.0f, 8.0f, 8.0f, 8.0f);
	column->AddNode(expandArea);
	addRect(expandArea, "Background", Color(45, 55, 70), Vector2::Zero);

	HBoxContainer* expandRow = MemNew<HBoxContainer>();
	expandRow->SetName("ExpandComparison");
	expandRow->SetSeparation(12.0f);
	expandArea->AddNode(expandRow);
	addRect(expandRow, "Fixed", Color(220, 80, 80), Vector2(80.0f, 28.0f));
	ColorRect* expandFill = addRect(expandRow, "ExpandFill", Color(80, 190, 120), Vector2(80.0f, 28.0f));
	expandFill->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	ColorRect* expandCenter = addRect(expandRow, "ExpandCenter", Color(220, 180, 70), Vector2(80.0f, 28.0f));
	expandCenter->SetHorizontalSizeFlags(Control::Size_Expand | Control::Size_ShrinkCenter);

	Label* corner = addLabel(content, "BottomRight", "BOTTOM RIGHT | fixed margins", 20);
	corner->SetHorizontalAlignment(HorizontalAlignment::Right);
	corner->SetVerticalAlignment(VerticalAlignment::Bottom);

	m_sceneTree->GetRoot()->AddNode(example);
}

void App::ShutdownScene()
{
	m_projectManagerErrorLabel = nullptr;
	m_uiRegenerateCount = 0;
	m_uiResizeSizeLabel = nullptr;
	m_editorCamera      = nullptr;
	m_isSlowMode        = false;

	MemDelete(m_sceneTree);
	m_sceneTree = nullptr;

	delete m_clock;
	m_clock = nullptr;
}

int MingEngine::Run(MingRunConfig const& config)
{
	if (config.generateCSharpBindings)
	{
		ClassDatabase::Startup();
		RegisterAllTypes();

		CSharpScriptGenerator csharpGenerator;
		bool const            success = csharpGenerator.GenerateCSharpBindings(config.csharpBindingsOutputDirectory);
		GUARANTEE_OR_DIE(success, "Failed to generate C# bindings.");

		ClassDatabase::Shutdown();
		return 0;
	}

	App app(config);
	g_app = &app;

	app.Startup();
	app.RunMainLoop();
	app.Shutdown();

	g_app = nullptr;
	return 0;
}
