#include "MingEngine/Editor/EditorApplication.hpp"

#include "MingEngine/Engine/Application/App.hpp"

#include "MingEngine/Editor/EditorController.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/Core/ClassDatabase.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Core/Clock.hpp"
#include "MingEngine/Engine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/Rgba8.hpp"

#include <vector>

void RegisterEditorTypes()
{
	ClassDatabase::RegisterClass<EditorNode>(false);
	ClassDatabase::RegisterClass<EditorGizmos>(false);
	ClassDatabase::RegisterClass<EditorController>(false);
}

AppEditorState::AppEditorState(IProjectModule& project) : m_project(project)
{
	m_editorClock = new Clock();
	m_sceneTree   = new SceneTree();

	BuildEditorScene();
}

AppEditorState::~AppEditorState()
{
	OnExit();

	delete m_sceneTree;
	m_sceneTree        = nullptr;
	m_editorController = nullptr;

	delete m_editorClock;
	m_editorClock = nullptr;
}

void AppEditorState::OnEnter()
{
	m_isSlowMode = false;
	if (m_editorClock != nullptr)
	{
		m_editorClock->SetTimeScale(1.0);
	}

	if (m_sceneTree != nullptr && m_editorController != nullptr)
	{
		m_sceneTree->SetWorldCamera(m_editorController->GetCamera());
	}
}

void AppEditorState::OnExit()
{
	m_isSlowMode = false;
	if (m_editorClock != nullptr)
	{
		m_editorClock->SetTimeScale(1.0);
	}
}

void AppEditorState::BuildEditorScene()
{
	if (m_sceneTree == nullptr)
	{
		return;
	}

	EditorNode* editorNode = new EditorNode();
	editorNode->SetName("EditorNode");
	m_sceneTree->GetRoot()->AddNode(editorNode);

	m_editorController = new EditorController();
	m_editorController->SetName("EditorController");
	Vec3 const editorStartPosition(-8.f, -8.f, 8.f);
	m_editorController->SetLocalPosition(editorStartPosition);
	m_editorController->SetLocalOrientation(EulerAngles::MakeFromForward(-editorStartPosition.GetNormalized()));
	m_sceneTree->GetRoot()->AddNode(m_editorController);

	IntVec2 screenDimensions = g_engine->m_window->GetClientDimensions();

	auto uiCamera = new Camera3D();
	uiCamera->SetName("UICamera");
	uiCamera->SetOrthogonal((float)screenDimensions.y, 0.f, 1.f);
	m_sceneTree->GetRoot()->AddNode(uiCamera);

	m_sceneTree->SetWorldCamera(m_editorController->GetCamera());
	m_sceneTree->SetUICamera(uiCamera);
}

void AppEditorState::HandleDebugInput()
{
	if (m_editorClock == nullptr)
	{
		return;
	}

	if (g_engine->m_input->WasKeyJustPressed('P'))
	{
		m_editorClock->TogglePause();
	}

	if (g_engine->m_input->WasKeyJustPressed('O'))
	{
		m_editorClock->StepSingleFrame();
	}

	if (g_engine->m_input->WasKeyJustPressed('T'))
	{
		m_isSlowMode = !m_isSlowMode;
		m_editorClock->SetTimeScale(m_isSlowMode ? 0.1 : 1.0);
	}

	if (g_engine->m_input->WasKeyJustPressed(KeyCodeEsc))
	{
		FireEvent("Quit");
	}
}

void AppEditorState::Update(float systemDeltaSeconds)
{
	HandleDebugInput();

	float const editorDeltaSeconds =
		m_editorClock != nullptr ? (float)m_editorClock->GetDeltaSeconds() : systemDeltaSeconds;
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->UpdateScene(editorDeltaSeconds);
	}

	UpdateDebugOverlay(systemDeltaSeconds);
}

void AppEditorState::UpdateDebugOverlay(float systemDeltaSeconds)
{
	if (m_editorClock == nullptr || m_editorController == nullptr)
	{
		return;
	}

	Vec3 editorControllerPosition           = m_editorController->GetWorldPosition();
	EulerAngles editorControllerOrientation = m_editorController->GetWorldOrientation();
	Vec3 editorControllerForward            = editorControllerOrientation.GetForwardDir_IFwd_JLeft_KUp();
	Vec2 screenDimensions                   = (Vec2)g_engine->m_window->GetClientDimensions();
	float frameRate                         = systemDeltaSeconds > 0.f ? 1.f / systemDeltaSeconds : 0.f;

	DebugAddScreenText(
		Stringf(
			"Time: %4.1f --- FPS: %3.0f --- Time Dilation: %1.2fx",
			(float)m_editorClock->GetTotalSeconds(),
			frameRate,
			(float)m_editorClock->GetTimeScale()),
		AABB2(
			Vec2(screenDimensions.x - 800.f, screenDimensions.y - 58.f),
			Vec2(screenDimensions.x - 10.f, screenDimensions.y - 10.f)),
		32.f,
		Vec2(1.f, 0.5f),
		0.f,
		Rgba8::White,
		Rgba8::White);

	DebugAddMessage(
		Stringf(
			"EditorController Pos: (%.2f, %.2f, %.2f) Fwd: (%.2f, %.2f, %.2f)",
			editorControllerPosition.x,
			editorControllerPosition.y,
			editorControllerPosition.z,
			editorControllerForward.x,
			editorControllerForward.y,
			editorControllerForward.z),
		0.f,
		Rgba8::White,
		Rgba8::White);
}

void AppEditorState::BeginFrame() {}

void AppEditorState::EndFrame()
{
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->FlushPendingNode();
	}
}

AppRuntimeState::AppRuntimeState()
{
	m_runtimeClock = new Clock();
	m_sceneTree    = new SceneTree();

	BuildRuntimeScene();
}

AppRuntimeState::~AppRuntimeState()
{
	OnExit();

	delete m_sceneTree;
	m_sceneTree = nullptr;

	delete m_runtimeClock;
	m_runtimeClock = nullptr;
}

void AppRuntimeState::OnEnter() {}

void AppRuntimeState::OnExit() {}

void AppRuntimeState::BuildRuntimeScene()
{
	if (m_sceneTree == nullptr)
	{
		return;
	}

	Node* runtimeScene = new Node();
	runtimeScene->SetName("RuntimeScene");

	auto directionalLight = new DirectionalLight3D();
	directionalLight->SetName("RuntimeDirectionalLight");
	directionalLight->SetDirection(Vec3(-1.f, -1.f, -1.f).GetNormalized());
	directionalLight->SetColor(Rgba8::White);
	directionalLight->SetIntensity(0.5f);
	runtimeScene->AddNode(directionalLight);

	m_sceneTree->ChangeScene(runtimeScene);
}

void AppRuntimeState::Update(float systemDeltaSeconds)
{
	float const runtimeDeltaSeconds =
		m_runtimeClock != nullptr ? (float)m_runtimeClock->GetDeltaSeconds() : systemDeltaSeconds;
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->UpdateScene(runtimeDeltaSeconds);
	}
}

void AppRuntimeState::BeginFrame() {}

void AppRuntimeState::EndFrame()
{
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->FlushPendingNode();
	}
}

AppStateMachine::AppStateMachine() = default;

AppStateMachine::~AppStateMachine() { Shutdown(); }

void AppStateMachine::Startup(AppStateType initialState, IProjectModule& project)
{
	Shutdown();

	m_editorState      = new AppEditorState(project);
	m_runtimeState     = nullptr;
	m_pendingStateType = initialState;
	m_hasPendingState  = true;

	ApplyPendingTransition();
}

void AppStateMachine::Shutdown()
{
	if (m_hasCurrentState)
	{
		AppStateBase* currentState = GetState(m_currentStateType);
		if (currentState != nullptr)
		{
			currentState->OnExit();
		}
	}

	delete m_editorState;
	m_editorState = nullptr;

	delete m_runtimeState;
	m_runtimeState = nullptr;

	m_currentStateType = AppStateType::Editor;
	m_pendingStateType = AppStateType::Editor;
	m_hasCurrentState  = false;
	m_hasPendingState  = false;
}

void AppStateMachine::RequestTransition(AppStateType stateID)
{
	if (m_hasCurrentState && stateID == m_currentStateType)
	{
		m_hasPendingState = false;
		return;
	}

	m_pendingStateType = stateID;
	m_hasPendingState  = true;
}

void AppStateMachine::ToggleEditorRuntime()
{
	RequestTransition(m_currentStateType == AppStateType::Editor ? AppStateType::Runtime : AppStateType::Editor);
}

void AppStateMachine::BeginFrame()
{
	ApplyPendingTransition();

	AppStateBase* currentState = GetState(m_currentStateType);
	if (currentState != nullptr)
	{
		currentState->BeginFrame();
	}
}

void AppStateMachine::Update(float deltaSeconds)
{
	AppStateBase* currentState = GetState(m_currentStateType);
	if (currentState != nullptr)
	{
		currentState->Update(deltaSeconds);
	}
}

void AppStateMachine::EndFrame()
{
	AppStateBase* currentState = GetState(m_currentStateType);
	if (currentState != nullptr)
	{
		currentState->EndFrame();
	}
}

AppStateBase* AppStateMachine::GetState(AppStateType stateID) const
{
	switch (stateID)
	{
	case AppStateType::Editor:
		return m_editorState;
	case AppStateType::Runtime:
		return m_runtimeState;
	default:
		return nullptr;
	}
}

void AppStateMachine::ApplyPendingTransition()
{
	if (!m_hasPendingState)
	{
		return;
	}

	AppStateBase* previousState = m_hasCurrentState ? GetState(m_currentStateType) : nullptr;
	AppStateBase* nextState     = GetState(m_pendingStateType);
	if (nextState == nullptr)
	{
		m_hasPendingState = false;
		return;
	}

	if (previousState != nullptr && previousState != nextState)
	{
		previousState->OnExit();
	}

	m_currentStateType = m_pendingStateType;
	m_hasCurrentState  = true;
	m_hasPendingState  = false;
	nextState->OnEnter();
}
