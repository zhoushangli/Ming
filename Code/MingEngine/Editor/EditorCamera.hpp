#pragma once

#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"

class Camera3D;

class EditorCamera : public Node3D
{
	MCLASS(EditorCamera, Node3D);

public:
	enum class EditorControlState
	{
		Pointer,
		FlyThrough,
	};

public:
	EditorCamera();
	~EditorCamera() override;

	static EditorCamera* Get();

	void OnProcess(float deltaSeconds) override;

	Camera3D*          GetCamera() const;
	EditorControlState GetControlState() const { return m_controlState; }
	Vec2               GetCursorClientPos() const { return m_lastCursorClientPos; }
	Vec2               GetCursorDelta() const { return m_cursorDelta; }

private:
	void UpdateControlState();
	void EnterControlState(EditorControlState nextState);
	void UpdateFlyThrough(float deltaSeconds);
	void UpdatePointer(float deltaSeconds);
	void UpdateCameraChild();

private:
	Camera3D* m_camera = nullptr;

	EditorControlState m_controlState        = EditorControlState::Pointer;
	Vec2               m_lastCursorClientPos = Vec2::Zero;
	Vec2               m_cursorDelta         = Vec2::Zero;

	static EditorCamera* s_instance;
};
