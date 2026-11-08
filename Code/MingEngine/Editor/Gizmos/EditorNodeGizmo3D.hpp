#pragma once

#include "MingEngine/Editor/Gizmos/GizmoComponent.hpp"

class EditorNodeGizmo3D : public EditorGizmoVisual3D
{
	MCLASS(EditorNodeGizmo3D, EditorGizmoVisual3D);

public:
	explicit EditorNodeGizmo3D(Node3D* target);
	~EditorNodeGizmo3D() override;

	ObjectID                    GetTargetID() const { return m_targetID; }
	Node3D*                     GetTarget() const;
	std::vector<Vector3> const& GetHandlePositions() const { return m_handles; }
	std::vector<Vector3> const& GetOutlineSegments() const { return m_segments; }
	bool                        IsDragging() const { return m_activeHandle >= 0; }

	void UpdateView(GizmoContext const& context);
	int  HitTest(GizmoContext const& context, float& distance, bool handlesOnly = false) const;
	void SetHoveredHandle(int handle);
	bool BeginHandleDrag(int handle, GizmoContext const& context);
	void UpdateHandleDrag(GizmoContext const& context);
	void EndHandleDrag(bool accept);

protected:
	virtual void               BuildShape(GizmoContext const& context)                                           = 0;
	virtual std::vector<float> GetParameters() const                                                             = 0;
	virtual char const*        GetHandleProperty(int handle) const                                               = 0;
	virtual bool               ComputeHandleValue(int handle, MathRaycastQuery3D const& ray, float& value) const = 0;
	virtual char const*        GetIconName() const                                                               = 0;

	Vector3 ToWorld(Vector3 const& local) const;
	void    AddLine(Vector3 const& a, Vector3 const& b);
	void    AddCircle(Vector3 const& center, Vector3 const& u, Vector3 const& v, float radius);
	bool    IntersectDragPlane(MathRaycastQuery3D const& ray, Vector3 const& normal, Vector3& local) const;
	bool    ProjectAxis(MathRaycastQuery3D const& ray, float& distance) const;

protected:
	Matrix4x4            m_basis;
	Vector3              m_origin;
	std::vector<Vector3> m_segments;
	std::vector<Vector3> m_handles;
	Color                m_color = Color(255, 215, 100, 255);
	Matrix4x4            m_dragBasis;
	Vector3              m_dragOrigin;
	Vector3              m_dragViewNormal;
	std::vector<float>   m_dragParameters;

private:
	ObjectID             m_targetID;
	ObjectID             m_dragSceneID  = ObjectID::Invalid;
	SceneTree*           m_dragTree     = nullptr;
	EditorGizmoVisual3D* m_icon         = nullptr;
	EditorGizmoVisual3D* m_handleVisual = nullptr;
	std::vector<float>   m_signature;
	int                  m_hoveredHandle = -1;
	int                  m_activeHandle  = -1;
	std::string          m_property;
	MethodBind const*    m_setter = nullptr;
	MethodBind const*    m_getter = nullptr;
	Variant              m_original;
	Variant              m_last;
};

class Light3DGizmo final : public EditorNodeGizmo3D
{
public:
	explicit Light3DGizmo(Node3D* target) : EditorNodeGizmo3D(target) {}

protected:
	void               BuildShape(GizmoContext const& context) override;
	std::vector<float> GetParameters() const override;
	char const*        GetHandleProperty(int handle) const override;
	bool               ComputeHandleValue(int handle, MathRaycastQuery3D const& ray, float& value) const override;
	char const*        GetIconName() const override;
};

class Camera3DGizmo final : public EditorNodeGizmo3D
{
public:
	explicit Camera3DGizmo(Node3D* target) : EditorNodeGizmo3D(target) {}

protected:
	void               BuildShape(GizmoContext const& context) override;
	std::vector<float> GetParameters() const override;
	char const*        GetHandleProperty(int handle) const override;
	bool               ComputeHandleValue(int handle, MathRaycastQuery3D const& ray, float& value) const override;
	char const*        GetIconName() const override { return "Camera3D"; }
};
