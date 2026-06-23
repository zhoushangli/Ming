#include "MingEngine/Scene/3D/Node3D.hpp"

#include "MingEngine/Scene/Core/SceneTree.hpp"

void Node3D::Reparent(Node* newParent, bool keepWorldTransform)
{
	if (keepWorldTransform)
	{
		Matrix4x4 worldTransformBefore = GetWorldTransform();
		Node::Reparent(newParent, keepWorldTransform);
		SetWorldTransform(worldTransformBefore);
	}
	else
	{
		Node::Reparent(newParent, keepWorldTransform);
	}
}

void Node3D::BindMethods()
{
	ClassDatabase::BindMethod("SetLocalTransform", &Node3D::SetLocalTransform);
	ClassDatabase::BindMethod("GetLocalTransform", &Node3D::GetLocalTransform);
	ClassDatabase::BindMethod("SetLocalPosition", &Node3D::SetLocalPosition);
	ClassDatabase::BindMethod("GetLocalPosition", &Node3D::GetLocalPosition);
	ClassDatabase::BindMethod("SetLocalOrientation", &Node3D::SetLocalOrientation);
	ClassDatabase::BindMethod("GetLocalOrientation", &Node3D::GetLocalOrientation);
	ClassDatabase::BindMethod("SetLocalScale", &Node3D::SetLocalScale);
	ClassDatabase::BindMethod("GetLocalScale", &Node3D::GetLocalScale);

	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Matrix4x4, "transform", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Storage),
		"SetLocalTransform",
		"GetLocalTransform");

	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Vec3, "position", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Inspector),
		"SetLocalPosition",
		"GetLocalPosition");

	ADD_PROPERTY(
		PropertyInfo(Variant::Type::EulerAngles, "rotation", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Inspector),
		"SetLocalOrientation",
		"GetLocalOrientation");

	ADD_PROPERTY(
		PropertyInfo(Variant::Type::Vec3, "scale", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Inspector),
		"SetLocalScale",
		"GetLocalScale");
}

Matrix4x4 Node3D::GetLocalTransform() const { return m_transform.GetMatrix(); }

Matrix4x4 Node3D::GetWorldTransform() const
{
	Matrix4x4 worldTransform = GetLocalTransform();
	Node3D* parent3D         = dynamic_cast<Node3D*>(m_data.m_parent);
	if (parent3D != nullptr)
	{
		Matrix4x4 parentTransform = parent3D->GetWorldTransform();
		parentTransform.Append(worldTransform);
		return parentTransform;
	}

	return worldTransform;
}

Matrix4x4 Node3D::GetWorldInverseTransform() const
{
	Matrix4x4 worldInverseTransform = m_transform.GetInverseMatrix();
	Node3D* parent3D                = dynamic_cast<Node3D*>(m_data.m_parent);
	if (parent3D != nullptr)
	{
		worldInverseTransform.Append(parent3D->GetWorldInverseTransform());
	}

	return worldInverseTransform;
}

void Node3D::SetLocalTransform(Matrix4x4 const& localTransform)
{
	m_transform.SetMatrix(localTransform);
	PropagateTransformChanged();
}

void Node3D::SetWorldTransform(Matrix4x4 const& worldTransform)
{
	Node3D* parent3D = dynamic_cast<Node3D*>(m_data.m_parent);
	if (parent3D != nullptr)
	{
		Matrix4x4 parentWorldInverse = parent3D->GetWorldInverseTransform();
		parentWorldInverse.Append(worldTransform);
		SetLocalTransform(parentWorldInverse);
	}
	else
	{
		SetLocalTransform(worldTransform);
	}
}

Vec3 Node3D::GetLocalPosition() const { return m_transform.GetPosition(); }

Vec3 Node3D::GetWorldPosition() const
{
	Matrix4x4 worldTransform = GetWorldTransform();
	return worldTransform.GetTranslation3D();
}

void Node3D::SetLocalPosition(Vec3 const& localPosition)
{
	m_transform.SetPosition(localPosition);
	PropagateTransformChanged();
}

Vec3 Node3D::GetLocalScale() const { return m_transform.GetScale(); }

void Node3D::SetLocalScale(Vec3 const& scale)
{
	m_transform.SetScale(scale);
	PropagateTransformChanged();
}

void Node3D::SetWorldPosition(Vec3 const& worldPosition)
{
	Node3D* parent3D = dynamic_cast<Node3D*>(m_data.m_parent);
	if (parent3D != nullptr)
	{
		Matrix4x4 parentWorldInverse = parent3D->GetWorldInverseTransform();
		SetLocalPosition(parentWorldInverse.TransformPosition3D(worldPosition));
	}
	else
	{
		SetLocalPosition(worldPosition);
	}
}

EulerAngles Node3D::GetLocalOrientation() const { return m_transform.GetOrientation(); }

EulerAngles Node3D::GetWorldOrientation() const
{
	Matrix4x4 worldTransform = GetWorldTransform();
	EulerAngles worldOrientation;
	worldOrientation.SetFromMatrix_IFwd_JLeft_KUp(worldTransform);

	return worldOrientation;
}

void Node3D::SetLocalOrientation(EulerAngles const& localOrientation)
{
	m_transform.SetOrientation(localOrientation);
	PropagateTransformChanged();
}

void Node3D::SetWorldOrientation(EulerAngles const& worldOrientation)
{
	Node3D* parent3D = dynamic_cast<Node3D*>(m_data.m_parent);
	if (parent3D != nullptr)
	{
		Matrix4x4 parentWorldInverse = parent3D->GetWorldInverseTransform();
		Matrix4x4 worldTransform     = worldOrientation.GetAsMatrix_IFwd_JLeft_KUp();
		parentWorldInverse.Append(worldTransform);
		EulerAngles localOrientation;
		localOrientation.SetFromMatrix_IFwd_JLeft_KUp(parentWorldInverse);
		SetLocalOrientation(localOrientation);
	}
	else
	{
		SetLocalOrientation(worldOrientation);
	}
}

void Node3D::SetWorldForward(Vec3 const& forward)
{
	EulerAngles currentOrientation    = GetWorldOrientation();
	EulerAngles desiredOrientation    = EulerAngles::MakeFromForward(forward);
	currentOrientation.m_pitchDegrees = desiredOrientation.m_pitchDegrees;
	currentOrientation.m_yawDegrees   = desiredOrientation.m_yawDegrees;
	SetWorldOrientation(currentOrientation);
}

Vec3 Node3D::GetWorldForward() const
{
	Vec3 forward, left, up;
	GetWorldOrientation().GetAsVectors_IFwd_JLeft_KUp(forward, left, up);
	return forward;
}

void Node3D::OnTransformChanged() {}

void Node3D::PropagateTransformChanged()
{
	SceneTree* sceneTree = GetSceneTree();
	if (sceneTree == nullptr)
	{
		return;
	}

	sceneTree->QueueTransformChangedNode(GetHandle());
	for (Node* child : GetChildren())
	{
		Node3D* child3D = dynamic_cast<Node3D*>(child);
		if (child3D != nullptr)
		{
			child3D->PropagateTransformChanged();
		}
	}
}

void Node3D::OnProcess(float deltaSeconds)
{
	bool const positionChanged    = m_velocity != Vec3::Zero;
	bool const orientationChanged = m_angularVelocity.m_yawDegrees != 0.f || m_angularVelocity.m_pitchDegrees != 0.f
									|| m_angularVelocity.m_rollDegrees != 0.f;
	if (!positionChanged && !orientationChanged)
	{
		return;
	}

	m_transform.SetPosition(m_transform.GetPosition() + m_velocity * deltaSeconds);
	m_transform.SetOrientation(m_transform.GetOrientation() + m_angularVelocity * deltaSeconds);
	PropagateTransformChanged();
}
