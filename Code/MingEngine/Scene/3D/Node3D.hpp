#pragma once

#include "MingEngine/Scene/3D/Transform3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"

class Node3D : public Node
{
	MCLASS(Node3D, Node);

	friend class SceneTree;

public:
	Node3D()           = default;
	~Node3D() override = default;

	void        Reparent(Node* newParent, bool keepWorldTransform = true) override;
	static void BindMethods();

	Matrix4x4 GetLocalTransform() const;
	Matrix4x4 GetWorldTransform() const;
	Matrix4x4 GetWorldInverseTransform() const;
	void      SetLocalTransform(Matrix4x4 const& localTransform);
	void      SetWorldTransform(Matrix4x4 const& worldTransform);

	Vector3 GetLocalPosition() const;
	Vector3 GetWorldPosition() const;
	void    SetLocalPosition(Vector3 const& localPosition);
	void    SetWorldPosition(Vector3 const& worldPosition);

	EulerAngles GetLocalOrientation() const;
	EulerAngles GetWorldOrientation() const;
	void        SetLocalOrientation(EulerAngles const& localOrientation);
	void        SetWorldOrientation(EulerAngles const& worldOrientation);
	void        SetWorldForward(Vector3 const& forward);
	Vector3     GetWorldForward() const;

	Vector3 GetLocalScale() const;
	void    SetLocalScale(Vector3 const& scale);
	Vector3 GetWorldScale() const;
	void    SetWorldScale(Vector3 const& scale);

protected:
	void OnNotification(int notification);

	virtual void OnTransformChanged();
	void         PropagateTransformChanged();

protected:
	Transform3D m_transform;
	Vector3     m_velocity        = Vector3::Zero;
	EulerAngles m_angularVelocity = EulerAngles::Zero;
};
