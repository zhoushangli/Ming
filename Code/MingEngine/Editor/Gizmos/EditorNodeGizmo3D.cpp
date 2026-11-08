#include "MingEngine/Editor/Gizmos/EditorNodeGizmo3D.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace Math;

namespace
{
float TanDegrees(float degrees)
{
	return std::tan(ConvertDegreesToRadians(degrees));
}

float PixelSize(Camera3D const& camera, Vector3 const& world, Vector2 dimensions)
{
	float const depth  = DotProduct3D(world - camera.GetWorldPosition(), camera.GetWorldForward());
	float const extent = camera.GetMode() == CameraMode::Orthographic
							 ? camera.GetSize()
							 : 2.f * std::max(camera.GetNearClip(), depth) * TanDegrees(camera.GetFovDegrees() * 0.5f);
	return std::max(0.00001f, extent / std::max(1.f, dimensions.y));
}

bool Project(Camera3D const& camera, Vector3 const& world, Vector2 dimensions, Vector2& point)
{
	Vector3 const p = camera.GetWorldInverseTransform().TransformPosition3D(world);
	if (p.x <= camera.GetNearClip())
	{
		return false;
	}

	float const height = camera.GetMode() == CameraMode::Orthographic
							 ? camera.GetSize()
							 : 2.f * p.x * TanDegrees(camera.GetFovDegrees() * 0.5f);
	point              = dimensions * 0.5f - Vector2(p.y, p.z) * (dimensions.y / std::max(0.00001f, height));
	return true;
}

void Quad(std::vector<Vertex>& vertices, Vector3 const& center, Vector3 const& right, Vector3 const& up)
{
	vertices.emplace_back(center - right + up, Color::White, Vector2(0.f, 0.f));
	vertices.emplace_back(center + right + up, Color::White, Vector2(1.f, 0.f));
	vertices.emplace_back(center + right - up, Color::White, Vector2(1.f, 1.f));
	vertices.emplace_back(center - right + up, Color::White, Vector2(0.f, 0.f));
	vertices.emplace_back(center + right - up, Color::White, Vector2(1.f, 1.f));
	vertices.emplace_back(center - right - up, Color::White, Vector2(0.f, 1.f));
}
}

EditorNodeGizmo3D::EditorNodeGizmo3D(Node3D* target) : m_targetID(target->GetObjectID())
{
	SetSerializable(false);
	m_icon = MemNew<EditorGizmoVisual3D>();
	m_icon->SetSerializable(false);
	AddNode(m_icon);
	m_handleVisual = MemNew<EditorGizmoVisual3D>();
	m_handleVisual->SetSerializable(false);
	AddNode(m_handleVisual);
}

EditorNodeGizmo3D::~EditorNodeGizmo3D()
{
	EndHandleDrag(false);
}

Node3D* EditorNodeGizmo3D::GetTarget() const
{
	Node3D* target = ObjectDatabase::GetInstance<Node3D>(m_targetID);
	return target != nullptr && !target->IsQueuedForDeletion() ? target : nullptr;
}

Vector3 EditorNodeGizmo3D::ToWorld(Vector3 const& local) const
{
	return m_origin + m_basis.TransformDirection3D(local);
}

void EditorNodeGizmo3D::AddLine(Vector3 const& a, Vector3 const& b)
{
	m_segments.push_back(ToWorld(a));
	m_segments.push_back(ToWorld(b));
}

void EditorNodeGizmo3D::AddCircle(Vector3 const& center, Vector3 const& u, Vector3 const& v, float radius)
{
	for (int i = 0; i < 64; ++i)
	{
		float const a = i * 360.f / 64.f;
		float const b = (i + 1) * 360.f / 64.f;
		AddLine(
			center + (u * CosDegrees(a) + v * SinDegrees(a)) * radius,
			center + (u * CosDegrees(b) + v * SinDegrees(b)) * radius);
	}
}

void EditorNodeGizmo3D::UpdateView(GizmoContext const& context)
{
	Node3D*    target = GetTarget();
	bool const valid  = target != nullptr && target->GetSceneTree() == context.m_sceneTree
						&& target->IsVisibleInHierarchy() && context.m_camera != nullptr;
	SetVisible(valid);
	if (!valid)
	{
		EndHandleDrag(false);
		return;
	}

	if (IsDragging() && (context.m_selectedNodeID != m_targetID || GetParameters() != m_dragParameters))
	{
		EndHandleDrag(false);
	}

	Matrix4x4 const    world       = target->GetWorldTransform();
	Matrix4x4 const    cameraWorld = context.m_camera->GetWorldTransform();
	std::vector<float> signature(world.m_values, world.m_values + 16);
	signature.insert(signature.end(), cameraWorld.m_values, cameraWorld.m_values + 16);
	auto parameters = GetParameters();
	signature.insert(signature.end(), parameters.begin(), parameters.end());
	signature.insert(
		signature.end(),
		{ context.m_clientDimensions.x,
		  context.m_clientDimensions.y,
		  context.m_camera->GetFovDegrees(),
		  context.m_camera->GetSize(),
		  static_cast<float>(context.m_camera->GetMode()),
		  context.m_selectedNodeID == m_targetID ? 1.f : 0.f });
	if (signature == m_signature)
	{
		return;
	}
	m_signature = std::move(signature);
	m_origin    = world.GetTranslation3D();
	m_basis     = world;
	m_basis.SetTranslation3D(Vector3::Zero);
	m_basis.Orthonormalize_XFwd_YLeft_ZUp2();
	m_segments.clear();
	m_handles.clear();
	bool const selected = context.m_selectedNodeID == m_targetID;
	if (selected)
	{
		BuildShape(context);
	}

	float const         scale = EditorNode::Get()->GetEditorTheme()->GetConstant("icon_size", "Button") / 20.f;
	float const         pixel = PixelSize(*context.m_camera, m_origin, context.m_clientDimensions);
	Vector3 const       left  = context.m_camera->GetWorldTransform().GetJBasis3D().GetNormalized();
	Vector3 const       up    = context.m_camera->GetWorldTransform().GetKBasis3D().GetNormalized();
	std::vector<Vertex> icon;
	Quad(icon, m_origin, -left * pixel * 12.f * scale, up * pixel * 12.f * scale);
	m_icon->UploadGeometry(icon);
	m_icon->SetGizmoTexture(EditorIcons::GetIconTexture(GetIconName()));
	m_icon->SetRenderPriority(900);

	m_verts.clear();
	for (size_t i = 0; i + 1 < m_segments.size(); i += 2)
	{
		if ((m_segments[i + 1] - m_segments[i]).GetLengthSquared() > 1e-10f)
		{
			AddVertsForCylinder3D(
				m_verts,
				m_segments[i],
				m_segments[i + 1],
				pixel * 0.7f * scale,
				m_color,
				AABB2::Unit,
				3);
		}
	}
	UploadGeometry(m_verts);
	SetRenderPriority(890);
	std::vector<Vertex> handles;
	for (size_t i = 0; i < m_handles.size(); ++i)
	{
		Color const color  = static_cast<int>(i) == m_activeHandle    ? Color(255, 170, 30, 255)
							 : static_cast<int>(i) == m_hoveredHandle ? Color(255, 235, 90, 255)
																	  : Color::White;
		float const radius = PixelSize(*context.m_camera, m_handles[i], context.m_clientDimensions) * 5.f * scale;
		AddVertsForSphere3D(handles, m_handles[i], radius, color, AABB2::Unit, 8, 4);
	}
	m_handleVisual->SetVisible(!handles.empty());
	m_handleVisual->UploadGeometry(handles);
	m_handleVisual->SetRenderPriority(2000);
	// Keep the outline tint synchronized with selection while retaining the billboard icon.
	// e.g. An unselected light keeps its icon visible without drawing its range outline.
	if (m_mesh.IsValid())
	{
		m_mesh->m_materialResource->SetTint(selected ? Color::White : Color(255, 255, 255, 0));
	}
}

int EditorNodeGizmo3D::HitTest(GizmoContext const& context, float& distance, bool handlesOnly) const
{
	if (!IsVisibleInHierarchy() || GetTarget() == nullptr || context.m_camera == nullptr)
	{
		return -1;
	}

	float const scale = EditorNode::Get()->GetEditorTheme()->GetConstant("icon_size", "Button") / 20.f;
	Vector2     screen;
	if (context.m_selectedNodeID == m_targetID)
	{
		for (size_t i = 0; i < m_handles.size(); ++i)
		{
			if (Project(*context.m_camera, m_handles[i], context.m_clientDimensions, screen)
				&& (screen - context.m_clientPos).GetLengthSquared() <= 64.f * scale * scale)
			{
				distance = (m_handles[i] - context.m_camera->GetWorldPosition()).GetLength();
				return static_cast<int>(i);
			}
		}
	}
	if (handlesOnly)
	{
		return -1;
	}

	if (Project(*context.m_camera, m_origin, context.m_clientDimensions, screen)
		&& std::abs(screen.x - context.m_clientPos.x) <= 12.f * scale
		&& std::abs(screen.y - context.m_clientPos.y) <= 12.f * scale)
	{
		distance = 0.f;
		return -2;
	}

	MathRaycastQuery3D const ray =
		context.m_camera->BuildRaycastFromMouse(context.m_clientPos, context.m_clientDimensions, 10000.f);
	float closest = distance;
	for (size_t i = 0; i + 1 < m_segments.size(); i += 2)
	{
		auto hit = RaycastVsCapsule3D(
			ray.m_startPos,
			ray.m_forwardNormal,
			ray.m_maxLength,
			m_segments[i],
			m_segments[i + 1],
			PixelSize(*context.m_camera, m_segments[i], context.m_clientDimensions) * 4.f * scale);
		if (hit.m_didImpact && hit.m_impactDist < closest)
		{
			closest = hit.m_impactDist;
		}
	}
	if (closest < distance)
	{
		distance = closest;
		return -2;
	}
	return -1;
}

void EditorNodeGizmo3D::SetHoveredHandle(int handle)
{
	if (m_hoveredHandle != handle)
	{
		m_hoveredHandle = handle;
		m_signature.clear();
	}
}

bool EditorNodeGizmo3D::BeginHandleDrag(int handle, GizmoContext const& context)
{
	Node3D*     target = GetTarget();
	char const* name   = GetHandleProperty(handle);
	Node*       scene  = context.m_sceneTree != nullptr ? context.m_sceneTree->GetScene() : nullptr;
	if (target == nullptr || name == nullptr || scene == nullptr || context.m_selectedNodeID != m_targetID)
	{
		return false;
	}

	PropertyInfo const* property = ClassDatabase::FindProperty(target->GetClassName(), name);
	if (property == nullptr || property->GetSetter() == nullptr || property->GetGetter() == nullptr)
	{
		return false;
	}
	m_setter         = property->GetSetter();
	m_getter         = property->GetGetter();
	m_property       = name;
	m_original       = m_getter->Invoke(target, {});
	m_last           = m_original;
	m_activeHandle   = handle;
	m_dragSceneID    = scene->GetObjectID();
	m_dragTree       = context.m_sceneTree;
	m_dragBasis      = m_basis;
	m_dragOrigin     = m_origin;
	m_dragViewNormal = context.m_camera->GetWorldForward();
	m_dragParameters = GetParameters();
	m_signature.clear();
	return true;
}

void EditorNodeGizmo3D::UpdateHandleDrag(GizmoContext const& context)
{
	Node3D* target = GetTarget();
	Node*   scene  = context.m_sceneTree != nullptr ? context.m_sceneTree->GetScene() : nullptr;
	if (!IsDragging())
	{
		return;
	}
	if (target == nullptr || target->GetSceneTree() != m_dragTree || context.m_sceneTree != m_dragTree
		|| scene == nullptr || scene->GetObjectID() != m_dragSceneID || context.m_selectedNodeID != m_targetID
		|| m_getter->Invoke(target, {}) != m_last || GetParameters() != m_dragParameters)
	{
		EndHandleDrag(false);
		return;
	}

	float value = 0.f;
	auto  ray   = context.m_camera->BuildRaycastFromMouse(context.m_clientPos, context.m_clientDimensions, 10000.f);
	if (ComputeHandleValue(m_activeHandle, ray, value) && std::isfinite(value))
	{
		m_setter->Invoke(target, { Variant(value) });
		target = GetTarget();
		if (target != nullptr)
		{
			m_last           = m_getter->Invoke(target, {});
			m_dragParameters = GetParameters();
		}
	}
}

void EditorNodeGizmo3D::EndHandleDrag(bool accept)
{
	if (!IsDragging())
	{
		return;
	}
	m_activeHandle = -1;
	m_signature.clear();
	Node3D* target = GetTarget();
	Node*   scene  = m_dragTree != nullptr ? m_dragTree->GetScene() : nullptr;
	if (target == nullptr || target->GetSceneTree() != m_dragTree || m_getter->Invoke(target, {}) != m_last)
	{
		return;
	}
	if (!accept || scene == nullptr || scene->GetObjectID() != m_dragSceneID)
	{
		m_setter->Invoke(target, { m_original });
		return;
	}
	EditorNode* editor = EditorNode::Get();
	if (m_last != m_original && editor != nullptr)
	{
		auto& history = editor->GetUndoRedo();
		if (history.CreateAction("Edit " + m_property, m_dragSceneID))
		{
			history.AddDoProperty(target, m_property, m_last);
			history.AddUndoProperty(target, m_property, m_original);
			history.AddDoMethod(editor, "MarkSceneDirty");
			history.AddUndoMethod(editor, "MarkSceneDirty");
			history.CommitAction(false);
		}
		editor->MarkSceneDirty();
	}
}

bool EditorNodeGizmo3D::IntersectDragPlane(MathRaycastQuery3D const& ray, Vector3 const& normal, Vector3& local) const
{
	auto hit = RaycastVsPlane3D(ray.m_startPos, ray.m_forwardNormal, ray.m_maxLength, m_dragOrigin, normal);
	if (!hit.m_didImpact)
	{
		return false;
	}
	Vector3 const delta = hit.m_impactPos - m_dragOrigin;
	local               = Vector3(
		DotProduct3D(delta, m_dragBasis.GetIBasis3D()),
		DotProduct3D(delta, m_dragBasis.GetJBasis3D()),
		DotProduct3D(delta, m_dragBasis.GetKBasis3D()));
	return true;
}

bool EditorNodeGizmo3D::ProjectAxis(MathRaycastQuery3D const& ray, float& distance) const
{
	Vector3 const axis        = m_dragBasis.GetIBasis3D();
	float const   dot         = DotProduct3D(axis, ray.m_forwardNormal);
	float const   denominator = 1.f - dot * dot;
	if (denominator < 1e-6f)
	{
		return false;
	}
	Vector3 const offset = ray.m_startPos - m_dragOrigin;
	distance             = (DotProduct3D(offset, axis) - dot * DotProduct3D(offset, ray.m_forwardNormal)) / denominator;
	return true;
}

std::vector<float> Light3DGizmo::GetParameters() const
{
	auto* light = dynamic_cast<Light3D*>(GetTarget());
	if (light == nullptr)
	{
		return {};
	}
	Color const        color  = light->GetColor();
	std::vector<float> values = { float(color.r), float(color.g), float(color.b) };
	if (auto* omni = dynamic_cast<OmniLight3D*>(light))
	{
		values.push_back(omni->GetRange());
	}
	if (auto* spot = dynamic_cast<SpotLight3D*>(light))
	{
		values.push_back(spot->GetRange());
		values.push_back(spot->GetSpotAngle());
	}
	return values;
}

char const* Light3DGizmo::GetIconName() const
{
	return dynamic_cast<OmniLight3D*>(GetTarget()) != nullptr   ? "OmniLight3D"
		   : dynamic_cast<SpotLight3D*>(GetTarget()) != nullptr ? "SpotLight3D"
																: "DirectionalLight3D";
}

void Light3DGizmo::BuildShape(GizmoContext const& context)
{
	auto* light         = static_cast<Light3D*>(GetTarget());
	m_color             = light->GetColor();
	int const brightest = std::max({ int(m_color.r), int(m_color.g), int(m_color.b) });
	m_color             = brightest > 0 ? m_color * (255.f / brightest) : Color::White;
	m_color.a           = 255;
	if (auto* omni = dynamic_cast<OmniLight3D*>(light))
	{
		float const radius = std::max(0.f, omni->GetRange());
		AddCircle(Vector3::Zero, Vector3::Forward, Vector3::Left, radius);
		AddCircle(Vector3::Zero, Vector3::Forward, Vector3::Up, radius);
		AddCircle(Vector3::Zero, Vector3::Left, Vector3::Up, radius);
		m_handles.push_back(m_origin - context.m_camera->GetWorldTransform().GetJBasis3D().GetNormalized() * radius);
	}
	else if (auto* spot = dynamic_cast<SpotLight3D*>(light))
	{
		float const radius = std::max(0.f, spot->GetRange());
		float const angle  = std::clamp(spot->GetSpotAngle(), 0.01f, 89.99f);
		float const depth  = radius * CosDegrees(angle);
		float const width  = radius * SinDegrees(angle);
		AddCircle(Vector3(depth, 0.f, 0.f), Vector3::Left, Vector3::Up, width);
		for (int i = 0; i < 8; ++i)
		{
			AddLine(Vector3::Zero, Vector3(depth, width * CosDegrees(i * 45.f), width * SinDegrees(i * 45.f)));
		}
		AddLine(Vector3::Zero, Vector3(radius, 0.f, 0.f));
		m_handles.push_back(ToWorld(Vector3(radius, 0.f, 0.f)));
		m_handles.push_back(ToWorld(Vector3(depth, width, 0.f)));
	}
	else
	{
		AddLine(Vector3::Zero, Vector3(1.5f, 0.f, 0.f));
		AddLine(Vector3(1.5f, 0.f, 0.f), Vector3(1.1f, 0.2f, 0.f));
		AddLine(Vector3(1.5f, 0.f, 0.f), Vector3(1.1f, -0.2f, 0.f));
		AddLine(Vector3(1.5f, 0.f, 0.f), Vector3(1.1f, 0.f, 0.2f));
		AddLine(Vector3(1.5f, 0.f, 0.f), Vector3(1.1f, 0.f, -0.2f));
	}
}

char const* Light3DGizmo::GetHandleProperty(int handle) const
{
	if (dynamic_cast<OmniLight3D*>(GetTarget()) != nullptr && handle == 0)
	{
		return "range";
	}
	if (dynamic_cast<SpotLight3D*>(GetTarget()) != nullptr)
	{
		return handle == 0 ? "range" : handle == 1 ? "spot_angle" : nullptr;
	}
	return nullptr;
}

bool Light3DGizmo::ComputeHandleValue(int handle, MathRaycastQuery3D const& ray, float& value) const
{
	if (dynamic_cast<OmniLight3D*>(GetTarget()) != nullptr)
	{
		Vector3 local;
		if (!IntersectDragPlane(ray, m_dragViewNormal, local))
		{
			return false;
		}
		value = local.GetLength();
		return true;
	}
	if (handle == 0)
	{
		if (!ProjectAxis(ray, value))
		{
			return false;
		}
		value = std::max(0.f, value);
		return true;
	}
	float const radius = std::max(0.0001f, m_dragParameters[3]);
	auto        error  = [&](float angle)
	{
		Vector3 const point =
			m_dragOrigin
			+ m_dragBasis.TransformDirection3D(Vector3(radius * CosDegrees(angle), radius * SinDegrees(angle), 0.f));
		float const along = std::clamp(DotProduct3D(point - ray.m_startPos, ray.m_forwardNormal), 0.f, ray.m_maxLength);
		return (point - ray.m_startPos - ray.m_forwardNormal * along).GetLengthSquared();
	};
	float best = 0.01f;
	for (int i = 1; i <= 90; ++i)
	{
		float const angle = std::min(89.99f, static_cast<float>(i));
		if (error(angle) < error(best))
		{
			best = angle;
		}
	}
	float low  = std::max(0.01f, best - 1.f);
	float high = std::min(89.99f, best + 1.f);
	for (int i = 0; i < 16; ++i)
	{
		float const a = low + (high - low) / 3.f;
		float const b = high - (high - low) / 3.f;
		if (error(a) < error(b))
		{
			high = b;
		}
		else
		{
			low = a;
		}
	}
	value = (low + high) * 0.5f;
	return true;
}

std::vector<float> Camera3DGizmo::GetParameters() const
{
	auto* camera = dynamic_cast<Camera3D*>(GetTarget());
	if (camera == nullptr)
	{
		return {};
	}
	IntVec2 const size =
		camera->GetViewport() != nullptr ? camera->GetViewport()->GetOutputResolution() : IntVec2(1, 1);
	return { static_cast<float>(camera->GetMode()),
			 camera->GetFovDegrees(),
			 camera->GetSize(),
			 static_cast<float>(size.x),
			 static_cast<float>(size.y) };
}

void Camera3DGizmo::BuildShape(GizmoContext const&)
{
	auto        parameters  = GetParameters();
	bool const  perspective = parameters[0] == static_cast<float>(CameraMode::Perspective);
	float const height =
		perspective ? TanDegrees(std::clamp(parameters[1], 1.f, 179.f) * 0.5f) : std::max(0.05f, parameters[2] * 0.5f);
	float const   width     = height * std::max(1.f, parameters[3]) / std::max(1.f, parameters[4]);
	Vector3 const corners[] = { Vector3(1.f, width, height),
								Vector3(1.f, -width, height),
								Vector3(1.f, -width, -height),
								Vector3(1.f, width, -height) };
	m_color                 = Color(125, 190, 255, 255);
	for (int i = 0; i < 4; ++i)
	{
		AddLine(corners[i], corners[(i + 1) % 4]);
		Vector3 const back = perspective ? Vector3::Zero : corners[i] - Vector3::Forward;
		AddLine(back, corners[i]);
		if (!perspective)
		{
			AddLine(back, corners[(i + 1) % 4] - Vector3::Forward);
		}
	}
	float const marker = std::max(0.1f, std::min(height, 0.5f));
	AddLine(Vector3(1.f, marker, height), Vector3(1.f, 0.f, height + marker));
	AddLine(Vector3(1.f, 0.f, height + marker), Vector3(1.f, -marker, height));
	m_handles.push_back(ToWorld(Vector3(1.f, 0.f, height)));
}

char const* Camera3DGizmo::GetHandleProperty(int handle) const
{
	if (handle != 0 || GetTarget() == nullptr)
	{
		return nullptr;
	}
	return static_cast<Camera3D*>(GetTarget())->GetMode() == CameraMode::Perspective ? "fov_degrees" : "size";
}

bool Camera3DGizmo::ComputeHandleValue(int, MathRaycastQuery3D const& ray, float& value) const
{
	auto hit = RaycastVsPlane3D(
		ray.m_startPos,
		ray.m_forwardNormal,
		ray.m_maxLength,
		m_dragOrigin + m_dragBasis.GetIBasis3D(),
		m_dragBasis.GetIBasis3D());
	if (!hit.m_didImpact)
	{
		return false;
	}
	float const height = std::abs(DotProduct3D(hit.m_impactPos - m_dragOrigin, m_dragBasis.GetKBasis3D()));
	if (m_dragParameters[0] == static_cast<float>(CameraMode::Perspective))
	{
		value = std::clamp(2.f * Atan2Degrees(height, 1.f), 1.f, 179.f);
	}
	else
	{
		value = std::clamp(2.f * height, 0.1f, 16384.f);
	}
	return true;
}
