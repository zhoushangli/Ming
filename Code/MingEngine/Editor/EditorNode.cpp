#include "MingEngine/Editor/EditorNode.hpp"

#include "MingEngine/Editor/EditorGizmos.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"
#include "MingEngine/Scene/Physics/Collider3D.hpp"
#include "MingEngine/Scene/Physics/NodeRaycastUtils.hpp"
#include "MingEngine/Scene/Physics/TriangleMeshCollider3D.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Math/AABB3.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"

namespace
{
float constexpr kEditorSelectionRaycastLength = 10000.f;
float constexpr kEditorColliderPadding        = 0.001f;

std::string FormatNodeHandle(NodeHandle handle)
{
	return Stringf("uid=%u index=%u", handle.GetUID(), handle.GetIndex());
}

AABB3 CalculateLocalBounds(std::vector<Vertex> const& verts)
{
	if (verts.empty())
	{
		return AABB3();
	}

	Vec3 mins = verts[0].m_position;
	Vec3 maxs = verts[0].m_position;
	for (Vertex const& vertex : verts)
	{
		mins.x = Min(mins.x, vertex.m_position.x);
		mins.y = Min(mins.y, vertex.m_position.y);
		mins.z = Min(mins.z, vertex.m_position.z);
		maxs.x = Max(maxs.x, vertex.m_position.x);
		maxs.y = Max(maxs.y, vertex.m_position.y);
		maxs.z = Max(maxs.z, vertex.m_position.z);
	}

	mins -= Vec3(kEditorColliderPadding, kEditorColliderPadding, kEditorColliderPadding);
	maxs += Vec3(kEditorColliderPadding, kEditorColliderPadding, kEditorColliderPadding);
	return AABB3(mins, maxs);
}
} // namespace

EditorNode* EditorNode::s_instance = nullptr;

NodeHandle EditorSelection::GetSelected() const { return m_selectedNodeHandle; }

void EditorSelection::SetSelected(NodeHandle handle)
{
	if (m_selectedNodeHandle == handle)
	{
		return;
	}

	NodeHandle const previousHandle = m_selectedNodeHandle;
	m_selectedNodeHandle            = handle;

	if (!previousHandle.IsValid() && m_selectedNodeHandle.IsValid())
	{
		DebugAddMessage(Stringf("Selected Node: %s", FormatNodeHandle(m_selectedNodeHandle).c_str()),
			5.f,
			Rgba8::White,
			Rgba8::White);
		return;
	}

	if (previousHandle.IsValid() && m_selectedNodeHandle.IsValid())
	{
		DebugAddMessage(Stringf("Selection Changed: %s -> %s",
							FormatNodeHandle(previousHandle).c_str(),
							FormatNodeHandle(m_selectedNodeHandle).c_str()),
			5.f,
			Rgba8::White,
			Rgba8::White);
		return;
	}

	if (previousHandle.IsValid() && !m_selectedNodeHandle.IsValid())
	{
		DebugAddMessage("Selection Cleared", 5.f, Rgba8::White, Rgba8::White);
	}
}

void EditorSelection::Clear() { SetSelected(NodeHandle::Invalid); }

EditorNode::EditorNode()
{
	s_instance = this;
	m_editorUI = new EditorUI();

	m_editorGizmos = new EditorGizmos();
	m_editorGizmos->SetName("EditorGizmos");
	m_editorGizmos->SetSerializable(false);
	AddNode(m_editorGizmos);

	SetReady(true);
	SetProcess(true);
}

EditorNode::~EditorNode()
{
	delete m_editorUI;
	m_editorUI = nullptr;

	if (s_instance == this)
	{
		s_instance = nullptr;
	}
}

EditorNode* EditorNode::Get() { return s_instance; }

EditorSelection& EditorNode::GetSelection() { return m_selection; }

EditorSelection const& EditorNode::GetSelection() const { return m_selection; }

void EditorNode::SaveSceneToFile(Node const* sceneRoot, std::string const& filename)
{
	PackedScene packedScene;

	if (!packedScene.Pack(sceneRoot))
	{
		return;
	}

	std::string outputFilename = filename;
	if (outputFilename.find('.') == std::string::npos)
	{
		outputFilename += ".json";
	}

	packedScene.SaveToFile(outputFilename);
}

void EditorNode::RegisterSelectableMesh(std::vector<Vertex> const& verts, NodeHandle ownerHandle)
{
	if (verts.empty() || !ownerHandle.IsValid())
	{
		return;
	}

	SceneTree* sceneTree = GetSceneTree();
	if (sceneTree == nullptr)
	{
		return;
	}

	Node3D* owner = dynamic_cast<Node3D*>(sceneTree->ResolveNode(ownerHandle));
	if (owner == nullptr)
	{
		return;
	}

	UnregisterSelectable(ownerHandle);

	AABBCollider3D*         coarseCollider = new AABBCollider3D(CalculateLocalBounds(verts));
	TriangleMeshCollider3D* fineCollider   = new TriangleMeshCollider3D();
	fineCollider->SetMesh(verts);
	coarseCollider->SetSerializable(false);
	fineCollider->SetSerializable(false);

	owner->AddNode(coarseCollider);
	owner->AddNode(fineCollider);

	EditorSelectable selectable;
	selectable.m_ownerHandle          = ownerHandle;
	selectable.m_coarseColliderHandle = coarseCollider->GetHandle();
	selectable.m_fineColliderHandle   = fineCollider->GetHandle();
	m_selectables[ownerHandle]        = selectable;
}

void EditorNode::UnregisterSelectable(NodeHandle ownerHandle)
{
	auto const foundSelectable = m_selectables.find(ownerHandle);
	if (foundSelectable == m_selectables.end())
	{
		return;
	}

	SceneTree* sceneTree = GetSceneTree();
	if (sceneTree != nullptr)
	{
		Node* coarseCollider = sceneTree->ResolveNode(foundSelectable->second.m_coarseColliderHandle);
		if (coarseCollider != nullptr)
		{
			coarseCollider->DeleteNode();
		}

		Node* fineCollider = sceneTree->ResolveNode(foundSelectable->second.m_fineColliderHandle);
		if (fineCollider != nullptr)
		{
			fineCollider->DeleteNode();
		}
	}

	m_selectables.erase(foundSelectable);
	if (m_selection.GetSelected() == ownerHandle)
	{
		m_selection.Clear();
	}
}

GameRaycastResult EditorNode::Raycast(
	RaycastInfo const& info, NodeHandle& outCoarseCollider, NodeHandle& outFineCollider, NodeHandle& outSelectNode)
{
	GameRaycastResult bestResult;
	bestResult.m_rayStartPos  = info.m_startPos;
	bestResult.m_rayFwdNormal = info.m_forwardNormal;
	bestResult.m_rayMaxLength = info.m_maxLength;
	outCoarseCollider         = NodeHandle::Invalid;
	outFineCollider           = NodeHandle::Invalid;
	outSelectNode             = NodeHandle::Invalid;

	SceneTree* sceneTree = GetSceneTree();
	if (sceneTree == nullptr)
	{
		return bestResult;
	}

	for (auto selectableIter = m_selectables.begin(); selectableIter != m_selectables.end();)
	{
		EditorSelectable const& selectable     = selectableIter->second;
		Node*                   coarseNode     = sceneTree->ResolveNode(selectable.m_coarseColliderHandle);
		Node*                   fineNode       = sceneTree->ResolveNode(selectable.m_fineColliderHandle);
		Node*                   selectNode     = sceneTree->ResolveNode(selectable.m_ownerHandle);
		Collider3D*             coarseCollider = dynamic_cast<Collider3D*>(coarseNode);
		Collider3D*             fineCollider   = dynamic_cast<Collider3D*>(fineNode);

		if (coarseCollider == nullptr || fineCollider == nullptr || selectNode == nullptr)
		{
			selectableIter = m_selectables.erase(selectableIter);
			continue;
		}

		if (!coarseCollider->m_isEnabled || !fineCollider->m_isEnabled)
		{
			++selectableIter;
			continue;
		}

		GameRaycastResult const coarseResult = coarseCollider->Raycast(info);
		if (!coarseResult.m_didImpact)
		{
			++selectableIter;
			continue;
		}

		GameRaycastResult const fineResult = fineCollider->Raycast(info);
		if (fineResult.m_didImpact && (!bestResult.m_didImpact || fineResult.m_impactDist < bestResult.m_impactDist))
		{
			bestResult        = fineResult;
			outCoarseCollider = selectable.m_coarseColliderHandle;
			outFineCollider   = selectable.m_fineColliderHandle;
			outSelectNode     = selectable.m_ownerHandle;
		}

		++selectableIter;
	}

	return bestResult;
}

void EditorNode::HandleSelectionClick(Camera3D const& cameraNode, Vec2 const& clientPos)
{
	SceneTree* sceneTree = GetSceneTree();
	if (sceneTree == nullptr || g_engine == nullptr || g_engine->m_window == nullptr)
	{
		m_selection.Clear();
		return;
	}

	Vec2 const    clientDimensions = (Vec2)g_engine->m_window->GetClientDimensions();
	float const   aspect           = clientDimensions.x / clientDimensions.y;
	CameraContext camera           = cameraNode.GetCamera(aspect);
	RaycastInfo raycastInfo = BuildRaycastFromMouse(camera, clientPos, clientDimensions, kEditorSelectionRaycastLength);
	NodeHandle  coarseCollider     = NodeHandle::Invalid;
	NodeHandle  fineCollider       = NodeHandle::Invalid;
	NodeHandle  selectNode         = NodeHandle::Invalid;
	GameRaycastResult const result = Raycast(raycastInfo, coarseCollider, fineCollider, selectNode);

	if (!result.m_didImpact || !selectNode.IsValid())
	{
		DebugAddMessage("Editor Pick Miss", 5.f, Rgba8::White, Rgba8::White);
		m_selection.Clear();
		return;
	}

	DebugAddMessage(Stringf("Editor Pick Hit: coarse=%s fine=%s selected=%s dist=%.2f pos=(%.2f, %.2f, %.2f)",
						FormatNodeHandle(coarseCollider).c_str(),
						FormatNodeHandle(fineCollider).c_str(),
						FormatNodeHandle(selectNode).c_str(),
						result.m_impactDist,
						result.m_impactPos.x,
						result.m_impactPos.y,
						result.m_impactPos.z),
		5.f,
		Rgba8::White,
		Rgba8::White);
	m_selection.SetSelected(selectNode);
}

void EditorNode::OnProcess([[maybe_unused]] float deltaSeconds)
{
	if (g_engine->m_input->WasKeyJustPressed('1'))
	{
		SceneTree* sceneTree = GetSceneTree();
		SaveSceneToFile(sceneTree->GetScene(), "EditorSavedScene");
	}

	if (g_engine->m_input->WasKeyJustPressed('2'))
	{
		SceneTree* sceneTree = GetSceneTree();

		PackedScene packedScene;
		packedScene.LoadFromFile("EditorSavedScene.json");

		Node* newSceneRoot = packedScene.Instantiate();

		if (newSceneRoot != nullptr)
		{
			sceneTree->ChangeScene(newSceneRoot);
		}
	}

	if (m_editorUI != nullptr)
	{
		EditorUIContext context;
		context.m_sceneTree = GetSceneTree();
		context.m_selection = &m_selection;
		m_editorUI->Render(context);
	}
}
