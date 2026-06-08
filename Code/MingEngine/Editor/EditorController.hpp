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
    ~EditorController() override = default;

    void Update(float deltaSeconds) override;
    Camera3D* GetCamera() const;

private:
    void UpdateControlState();
    void EnterControlState(EditorControlState nextState);
    void UpdateFlyThrough(float deltaSeconds);
    void UpdatePointer(float deltaSeconds);
    void UpdateCameraChild();

    void BeginPointerClick(Vec2 const& cursorPos);
    void UpdatePointerDrag(Vec2 const& cursorPos);
    void EndPointerClick(Vec2 const& cursorPos);

private:
    Camera3D* m_camera = nullptr;

    EditorControlState m_controlState = EditorControlState::Pointer;
    Vec2 m_dragStartClientPos = Vec2::Zero;
    Vec2 m_lastCursorClientPos = Vec2::Zero;
    bool m_isDragging = false;
};
