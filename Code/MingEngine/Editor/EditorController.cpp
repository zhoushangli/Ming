#include "MingEngine/Editor/EditorController.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"

#include <cmath>

constexpr float kMouseLookSensitivity           = 0.125f;
constexpr float kMoveSpeedUnitsPerSec           = 4.f;
constexpr float kSprintMultiplier               = 3.f;
constexpr float kMaxPitchDeg                    = 85.f;
constexpr float kPointerDragStartDistancePixels = 4.f;

EditorController::EditorController()
{
	SetLocalPosition(Vec3(-2.f, 0.f, 0.f));

	m_camera = new Camera3D(60.f, 0.1f, 100.f);
	AddNode(m_camera);

	SetReady(true);
	SetProcess(true);
}

void EditorController::OnProcess(float deltaSeconds)
{
	// PCG editor commands will move to a scene-local world/editor manager.

	bool const shouldReset = g_engine->m_input->WasKeyJustPressed('H');
	if (shouldReset)
	{
		SetLocalPosition(Vec3(0.f, 0.f, 0.f));
		SetLocalOrientation(EulerAngles(0.f, 0.f, 0.f));
		m_velocity        = Vec3(0.f, 0.f, 0.f);
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

void EditorController::UpdateControlState()
{
	InputSystem* input                    = g_engine->m_input;
	bool const isConsoleOpen              = (g_engine->m_devConsole != nullptr) && g_engine->m_devConsole->IsOpen();
	EditorControlState const desiredState = !isConsoleOpen && input->IsKeyDown(KeyCodeRightMouse)
												? EditorControlState::FlyThrough
												: EditorControlState::Pointer;

	EnterControlState(desiredState);
}

void EditorController::EnterControlState(EditorControlState nextState)
{
	if (m_controlState == nextState)
	{
		return;
	}

	if (m_controlState == EditorControlState::Pointer)
	{
		m_isDragging = false;
	}

	m_controlState = nextState;

	if (m_controlState == EditorControlState::FlyThrough)
	{
		g_engine->m_input->SetCursorMode(CursorMode::FPS);
		g_engine->m_input->ClearCursorDelta();
	}
	else
	{
		g_engine->m_input->SetCursorMode(CursorMode::POINTER);
		g_engine->m_input->ClearCursorDelta();
		m_lastCursorClientPos = g_engine->m_input->GetCursorClientPosition();
	}
}

void EditorController::UpdateFlyThrough(float deltaSeconds)
{
	InputSystem* input = g_engine->m_input;
	Vec2 mouseDelta    = input->GetCursorClientDelta();

	EulerAngles orientation = GetLocalOrientation();
	orientation.m_yawDegrees -= mouseDelta.x * kMouseLookSensitivity;
	orientation.m_pitchDegrees += mouseDelta.y * kMouseLookSensitivity;
	orientation.m_pitchDegrees = GetClamped(orientation.m_pitchDegrees, -kMaxPitchDeg, kMaxPitchDeg);
	orientation.m_rollDegrees  = 0.f;
	SetLocalOrientation(orientation);

	Vec2 moveInput = Vec2::Zero;
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

	Vec3 forward = Vec3(0.f, 0.f, 0.f);
	Vec3 left    = Vec3(0.f, 0.f, 0.f);
	Vec3 up      = Vec3(0.f, 0.f, 0.f);
	orientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

	Vec3 right       = -left;
	Vec3 movementDir = right * moveInput.x + forward * moveInput.y + Vec3(0.f, 0.f, verticalInput);

	float moveLenSq = movementDir.x * movementDir.x + movementDir.y * movementDir.y + movementDir.z * movementDir.z;
	if (moveLenSq > 1.f)
	{
		float invLen = 1.f / sqrtf(moveLenSq);
		movementDir *= invLen;
	}

	bool const isSprinting = input->IsKeyDown(KeyCodeShift);
	float moveSpeed        = kMoveSpeedUnitsPerSec;
	if (isSprinting)
	{
		moveSpeed *= kSprintMultiplier;
	}

	m_velocity = movementDir * moveSpeed;
	SetLocalPosition(GetLocalPosition() + m_velocity * deltaSeconds);
}

void EditorController::UpdatePointer([[maybe_unused]] float deltaSeconds)
{
	InputSystem* input   = g_engine->m_input;
	Vec2 const cursorPos = input->GetCursorClientPosition();
	if (input->WasKeyJustPressed(KeyCodeLeftMouse))
	{
		BeginPointerClick(cursorPos);
	}

	if (input->IsKeyDown(KeyCodeLeftMouse))
	{
		Vec2 const dragDelta = cursorPos - m_dragStartClientPos;
		if (!m_isDragging
			&& dragDelta.GetLengthSquared() >= kPointerDragStartDistancePixels * kPointerDragStartDistancePixels)
		{
			m_isDragging = true;
		}

		if (m_isDragging)
		{
			UpdatePointerDrag(cursorPos);
		}
	}

	if (input->WasKeyJustReleased(KeyCodeLeftMouse))
	{
		EndPointerClick(cursorPos);
	}

	m_velocity            = Vec3::Zero;
	m_lastCursorClientPos = cursorPos;
}

void EditorController::UpdateCameraChild()
{
	if (m_camera == nullptr)
	{
		return;
	}

	m_camera->SetLocalPosition(Vec3::Zero);
	m_camera->SetLocalOrientation(EulerAngles::Zero);
}

void EditorController::BeginPointerClick(Vec2 const& cursorPos)
{
	m_dragStartClientPos  = cursorPos;
	m_lastCursorClientPos = cursorPos;
	m_isDragging          = false;

	// Viewport picking is paused while editor selection is driven by the Scene panel.
	// The raycast selection path remains available for a later picking redesign.
	// EditorNode* editorNode = EditorNode::Get();
	// if (editorNode != nullptr && m_camera != nullptr)
	// {
	// 	editorNode->HandleSelectionClick(*m_camera, cursorPos);
	// }
}

void EditorController::UpdatePointerDrag(Vec2 const& cursorPos)
{
	m_lastCursorClientPos = cursorPos;
}

void EditorController::EndPointerClick(Vec2 const& cursorPos)
{
	m_lastCursorClientPos = cursorPos;
	m_isDragging          = false;
}


Camera3D* EditorController::GetCamera() const { return m_camera; }
