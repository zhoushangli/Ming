#include "MingEngine/Editor/EditorNode.hpp"

#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"
#include "MingEngine/Scene/Physics/Collider3D.hpp"
#include "MingEngine/Scene/Physics/NodeRaycastUtils.hpp"
#include "MingEngine/Scene/Physics/TriangleMeshCollider3D.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Math/AABB3.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"

#include "ThirdParty/imgui/imgui.h"

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

NodeHandle EditorSelection::GetSelectedNodeHandle() const { return m_selectedNodeHandle; }

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

EditorNode::EditorNode() { s_instance = this; }

EditorNode::~EditorNode()
{
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
	if (m_selection.GetSelectedNodeHandle() == ownerHandle)
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

void EditorNode::Update([[maybe_unused]] float deltaSeconds)
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

	RenderEditorUI();
}

void EditorNode::RenderEditorUI()
{
	RenderMainMenuBar();
	RenderDockSpace();

	if (m_showScenePanel)
	{
		RenderScenePanel();
	}
	if (m_showFileSystemPanel)
	{
		RenderFileSystemPanel();
	}
	if (m_showViewportPanel)
	{
		RenderViewportPanel();
	}
	if (m_showInspectorPanel)
	{
		RenderInspectorPanel();
	}
	if (m_showOutputPanel)
	{
		RenderOutputPanel();
	}
}

void EditorNode::RenderMainMenuBar()
{
	if (!ImGui::BeginMainMenuBar())
	{
		return;
	}

	if (ImGui::BeginMenu("Scene"))
	{
		ImGui::MenuItem("New Scene");
		ImGui::MenuItem("Open Scene...");
		ImGui::Separator();
		ImGui::MenuItem("Save Scene");
		ImGui::MenuItem("Save Scene As...");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Project"))
	{
		ImGui::MenuItem("Project Settings...");
		ImGui::MenuItem("Reload Project");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Debug"))
	{
		ImGui::MenuItem("Start");
		ImGui::MenuItem("Pause");
		ImGui::MenuItem("Stop");
		ImGui::MenuItem("Step Frame");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Editor"))
	{
		ImGui::MenuItem("Editor Settings...");
		ImGui::MenuItem("Reset Layout");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Window"))
	{
		ImGui::MenuItem("Scene", nullptr, &m_showScenePanel);
		ImGui::MenuItem("FileSystem", nullptr, &m_showFileSystemPanel);
		ImGui::MenuItem("Viewport", nullptr, &m_showViewportPanel);
		ImGui::MenuItem("Inspector", nullptr, &m_showInspectorPanel);
		ImGui::MenuItem("Output", nullptr, &m_showOutputPanel);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Help"))
	{
		ImGui::MenuItem("About Project QingChen");
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
}

void EditorNode::RenderDockSpace()
{
	ImGuiViewport const* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
	windowFlags |= ImGuiWindowFlags_NoTitleBar;
	windowFlags |= ImGuiWindowFlags_NoCollapse;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	windowFlags |= ImGuiWindowFlags_NoNavFocus;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::Begin("QingChen Editor DockSpace", nullptr, windowFlags);
	ImGui::PopStyleVar(2);

	ImGuiID dockspaceId = ImGui::GetID("QingChenEditorDockSpace");
	ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_None);

	ImGui::End();
}

void EditorNode::RenderScenePanel()
{
	ImGui::Begin("Scene", &m_showScenePanel);

	static char filterNodes[64] = "";
	ImGui::InputTextWithHint("##FilterNodes", "Filter Nodes", filterNodes, sizeof(filterNodes));
	ImGui::Separator();

	if (ImGui::TreeNodeEx("WorldRoot", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TreeNodeEx("Player", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		ImGui::TreeNodeEx("Camera", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		ImGui::TreeNodeEx("DirectionalLight", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		if (ImGui::TreeNodeEx("MeshRoot", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TreeNodeEx("Cube_01", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TreeNodeEx("Cube_02", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TreePop();
		}
		ImGui::TreePop();
	}

	ImGui::End();
}

void EditorNode::RenderFileSystemPanel()
{
	ImGui::Begin("FileSystem", &m_showFileSystemPanel);

	ImGui::TextUnformatted("res://");
	ImGui::Separator();
	static char filterFiles[64] = "";
	ImGui::InputTextWithHint("##FilterFiles", "Filter Files", filterFiles, sizeof(filterFiles));

	if (ImGui::TreeNodeEx("Data", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TreeNodeEx("Images", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		if (ImGui::TreeNodeEx("Models", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TreeNodeEx("Cube.fbx", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TreeNodeEx("Character.fbx", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
			ImGui::TreePop();
		}
		ImGui::TreeNodeEx("Shaders", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		ImGui::TreeNodeEx("Audio", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
		ImGui::TreePop();
	}

	ImGui::End();
}

void EditorNode::RenderViewportPanel()
{
	ImGui::Begin("Viewport", &m_showViewportPanel);

	ImGui::Button("Select");
	ImGui::SameLine();
	ImGui::Button("Move");
	ImGui::SameLine();
	ImGui::Button("Rotate");
	ImGui::SameLine();
	ImGui::Button("Scale");
	ImGui::SameLine();
	static bool snap = false;
	ImGui::Checkbox("Snap", &snap);
	ImGui::Separator();

	// We should fire event first because we will get viewport->GetViewportInfo().m_viewportOutputTexture next
	// And we don't want to get the old viewport output texture after resized
	ImVec2 const availableSize = ImGui::GetContentRegionAvail();
	int width  = static_cast<int>(availableSize.x);
	int height = static_cast<int>(availableSize.y);
	width      = Max(width, 1);
	height     = Max(height, 1);
	IntVec2 panelDimensions(width, height);
	if (panelDimensions != m_viewportPanelDimension)
	{
		m_viewportPanelDimension = panelDimensions;
		EventArgs args;
		args.SetValue("width", std::to_string(width));
		args.SetValue("height", std::to_string(height));
		FireEvent("EditorViewportResized", args);
	}

	Viewport* viewport = dynamic_cast<Viewport*>(GetSceneTree()->GetRoot());
	if (viewport != nullptr)
	{
		Texture* viewportTexture = viewport->GetViewportInfo().m_viewportOutputTexture;
		if (viewportTexture != nullptr)
		{
			ImTextureID  texID  = (ImTextureID)(intptr_t)viewportTexture->GetShaderResourceView();
			ImTextureRef texRef = ImTextureRef(texID);
			ImGui::Image(texRef, availableSize);
		}
	}

	ImGui::End();
}

void EditorNode::RenderInspectorPanel()
{
	ImGui::Begin("Inspector", &m_showInspectorPanel);

	ImGui::TextUnformatted("Selected: Player");
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static float position[3] = { 0.f, 0.f, 0.f };
		static float rotation[3] = { 0.f, 0.f, 0.f };
		static float scale[3]    = { 1.f, 1.f, 1.f };

		ImGui::DragFloat3("Position", position, 0.1f, 0.0f, 0.f, "%.1f");
		ImGui::DragFloat3("Rotation", rotation, 0.1f, 0.0f, 0.f, "%.1f");
		ImGui::DragFloat3("Scale", scale, 0.1f, 0.0f, 0.f, "%.1f");
	}

	if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen))
	{
		static bool visible = true;
		ImGui::Checkbox("Visible", &visible);
		ImGui::TextUnformatted("Material: Default");
		ImGui::TextUnformatted("Mesh: Cube");
	}

	ImGui::End();
}

void EditorNode::RenderOutputPanel()
{
	ImGui::Begin("Output", &m_showOutputPanel);

	if (ImGui::BeginTabBar("OutputTabs"))
	{
		if (ImGui::BeginTabItem("Output"))
		{
			ImGui::TextUnformatted("[Info] Editor started");
			ImGui::TextUnformatted("[Info] Loaded scene: Untitled");
			ImGui::TextUnformatted("[Warning] Viewport is a placeholder in this phase");
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Console"))
		{
			ImGui::TextUnformatted("> Console input placeholder");
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Profiler"))
		{
			ImGui::TextUnformatted("Frame: -- ms");
			ImGui::TextUnformatted("Draw Calls: --");
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
}
