#pragma once

#include "MingEngine/Scene/3D/Node3D.hpp"

#include "MingEngine/Engine/Math/Vec2.hpp"

class Camera3D;

class EditorController : public Node3D
{
	MCLASS(EditorController, Node3D);

public:
	enum class EditorControlState
	{
		Pointer,
		FlyThrough,
	};

public:
	EditorController();
	~EditorController() override;

	static EditorController* Get();

	void OnProcess(float deltaSeconds) override;
	Camera3D* GetCamera() const;

private:
	void UpdateControlState();
	void EnterControlState(EditorControlState nextState);
	void UpdateFlyThrough(float deltaSeconds);
	void UpdatePointer(float deltaSeconds);
	void UpdateCameraChild();

private:
	Camera3D* m_camera = nullptr;

	EditorControlState m_controlState = EditorControlState::Pointer;
	Vec2 m_lastCursorClientPos        = Vec2::Zero;

	static EditorController* s_instance;
};
