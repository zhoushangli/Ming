#pragma once

#include "MingEngine/Scene/3D/Transform3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"

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
	void      SetLocalTransform(Matrix4x4 const& localTransform);
	void      SetWorldTransform(Matrix4x4 const& worldTransform);

	Vec3 GetLocalPosition() const;
	Vec3 GetWorldPosition() const;
	void SetLocalPosition(Vec3 const& localPosition);
	void SetWorldPosition(Vec3 const& worldPosition);

	EulerAngles GetLocalOrientation() const;
	EulerAngles GetWorldOrientation() const;
	void        SetLocalOrientation(EulerAngles const& localOrientation);
	void        SetWorldOrientation(EulerAngles const& worldOrientation);
	void        SetWorldForward(Vec3 const& forward);
	Vec3        GetWorldForward() const;

	Vec3 GetLocalScale() const;
	void SetLocalScale(Vec3 const& scale);

protected:
	void OnNotification(int notification);

	virtual void OnTransformChanged();
	void         PropagateTransformChanged();
	Matrix4x4    GetWorldInverseTransform() const;

protected:
	Transform3D m_transform;
	Vec3        m_velocity        = Vec3::Zero;
	EulerAngles m_angularVelocity = EulerAngles::Zero;
};
