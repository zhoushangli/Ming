#include "MingEngine/Editor/EditorCamera.hpp"

#include "MingEngine/Scene/3D/Camera3D.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/EngineService/EngineService.hpp"

using namespace Math;

constexpr float kMouseLookSensitivity = 0.125f;
constexpr float kMoveSpeedUnitsPerSec = 4.f;
constexpr float kSprintMultiplier     = 3.f;
constexpr float kMaxPitchDeg          = 85.f;

EditorCamera* EditorCamera::s_instance = nullptr;

EditorCamera::EditorCamera()
{
	s_instance = this;

	m_camera = MemNew<Camera3D>(60.f, 0.1f, 100.f);
	AddNode(m_camera);

	SetReady(true);
	SetProcess(true);
}

EditorCamera::~EditorCamera()
{
	if (s_instance == this)
	{
		s_instance = nullptr;
	}
}

EditorCamera* EditorCamera::Get() { return s_instance; }

void EditorCamera::OnProcess(float deltaSeconds)
{
	// PCG editor commands will move to a scene-local world/editor manager.

	bool const shouldReset = g_engine->m_inputSystem->WasKeyJustPressed('H');
	if (shouldReset)
	{
		SetLocalPosition(Vector3(0.f, 0.f, 0.f));
		SetLocalOrientation(EulerAngles(0.f, 0.f, 0.f));
		m_velocity        = Vector3(0.f, 0.f, 0.f);
		m_angularVelocity = EulerAngles(0.f, 0.f, 0.f);
	}

	UpdateControlState();

	switch (m_controlState)
	{
	case EditorControlState::Pointer:
		UpdatePointer(deltaSeconds);
		break;

	case EditorControlState::FlyThrough:
		UpdateFlyThrough(deltaSeconds);
		break;
	}

	UpdateCameraChild();
}

void EditorCamera::UpdateControlState()
{
	InputSystem* input = g_engine->m_inputSystem;
	bool const   isConsoleOpen =
		(g_engineService != nullptr) && (g_engineService->m_console != nullptr) && g_engineService->m_console->IsOpen();
	EditorControlState const desiredState = !isConsoleOpen && input->IsKeyDown(KeyCode::RightMouse)
												? EditorControlState::FlyThrough
												: EditorControlState::Pointer;

	EnterControlState(desiredState);
}

void EditorCamera::EnterControlState(EditorControlState nextState)
{
	if (m_controlState == nextState)
	{
		return;
	}

	m_controlState = nextState;

	if (m_controlState == EditorControlState::FlyThrough)
	{
		g_engine->m_inputSystem->SetCursorMode(CursorMode::FPS);
		g_engine->m_inputSystem->ClearCursorDelta();
	}
	else
	{
		g_engine->m_inputSystem->SetCursorMode(CursorMode::POINTER);
		g_engine->m_inputSystem->ClearCursorDelta();
		m_lastCursorClientPos = Vector2(g_engine->m_inputSystem->GetCursorClientPosition());
	}
}

void EditorCamera::UpdateFlyThrough(float deltaSeconds)
{
	InputSystem* input      = g_engine->m_inputSystem;
	Vector2      mouseDelta = Vector2(input->GetCursorClientDelta());

	EulerAngles orientation = GetLocalOrientation();
	orientation.m_yawDegrees -= mouseDelta.x * kMouseLookSensitivity;
	orientation.m_pitchDegrees += mouseDelta.y * kMouseLookSensitivity;
	orientation.m_pitchDegrees = GetClamped(orientation.m_pitchDegrees, -kMaxPitchDeg, kMaxPitchDeg);
	orientation.m_rollDegrees  = 0.f;
	SetLocalOrientation(orientation);

	Vector2 moveInput = Vector2::Zero;
	if (input->IsKeyDown('W'))
	{
		moveInput.y += 1.f;
	}
	if (input->IsKeyDown('S'))
	{
		moveInput.y -= 1.f;
	}
	if (input->IsKeyDown('A'))
	{
		moveInput.x -= 1.f;
	}
	if (input->IsKeyDown('D'))
	{
		moveInput.x += 1.f;
	}
	moveInput = moveInput.GetClamped(1.f);

	float verticalInput = 0.f;
	if (input->IsKeyDown('Z'))
	{
		verticalInput -= 1.f;
	}
	if (input->IsKeyDown('C'))
	{
		verticalInput += 1.f;
	}
	verticalInput = GetClamped(verticalInput, -1.f, 1.f);

	Vector3 forward = Vector3(0.f, 0.f, 0.f);
	Vector3 left    = Vector3(0.f, 0.f, 0.f);
	Vector3 up      = Vector3(0.f, 0.f, 0.f);
	orientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

	Vector3 right       = -left;
	Vector3 movementDir = right * moveInput.x + forward * moveInput.y + Vector3(0.f, 0.f, verticalInput);

	float moveLenSq = movementDir.x * movementDir.x + movementDir.y * movementDir.y + movementDir.z * movementDir.z;
	if (moveLenSq > 1.f)
	{
		float invLen = 1.f / sqrtf(moveLenSq);
		movementDir *= invLen;
	}

	bool const isSprinting = input->IsKeyDown(KeyCode::LeftShift);
	float      moveSpeed   = kMoveSpeedUnitsPerSec;
	if (isSprinting)
	{
		moveSpeed *= kSprintMultiplier;
	}

	m_velocity = movementDir * moveSpeed;
	SetLocalPosition(GetLocalPosition() + m_velocity * deltaSeconds);
}

void EditorCamera::UpdatePointer([[maybe_unused]] float deltaSeconds)
{
	InputSystem*  input     = g_engine->m_inputSystem;
	Vector2 const cursorPos = Vector2(input->GetCursorClientPosition());
	m_cursorDelta           = cursorPos - m_lastCursorClientPos;
	m_lastCursorClientPos   = cursorPos;

	m_velocity = Vector3::Zero;
}

void EditorCamera::UpdateCameraChild()
{
	if (m_camera == nullptr)
	{
		return;
	}

	m_camera->SetLocalPosition(Vector3::Zero);
	m_camera->SetLocalOrientation(EulerAngles::Zero);
}

Camera3D* EditorCamera::GetCamera() const { return m_camera; }
