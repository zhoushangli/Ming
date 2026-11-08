#pragma once

#include "MingEngine/Scene/Core/PackedScene.hpp"

template <typename Frame, typename Click, typename Check>
void VerifyInspectorVisibility(SceneTree& tree, EditorNode* editor, Frame frame, Click click, Check check)
{
	auto* scene = MemNew<Node3D>();
	scene->SetName("InspectorVisibilityChecks");
	auto* group = MemNew<Node3D>();
	group->SetName("HiddenGroup");
	scene->AddNode(group);
	auto* mesh = MemNew<MeshInstance3D>();
	mesh->SetName("VisibilityMesh");
	mesh->SetLocalPosition(Vector3(2.f, 3.f, 4.f));
	group->AddNode(mesh);
	auto* plain = MemNew<Node>();
	plain->SetName("PlainNode");
	scene->AddNode(plain);
	auto* control = MemNew<Control>();
	control->SetName("VisibleControl");
	scene->AddNode(control);
	tree.ChangeScene(scene);
	tree.FlushPendingNode();
	editor->GetSelection().SetSelected(mesh->GetObjectID());
	frame();
	frame();

	auto& inspector = editor->m_editorUI->GetPanel<InspectorPanel>();
	auto* position = FindNamedUI(&inspector, "position");
	auto* meshField = FindNamedUI(&inspector, "mesh");
	auto* script = FindNamedUI(&inspector, "script");
	auto* positionReset = dynamic_cast<Button*>(FindNamedUI(position, "RevertProperty"));
	auto* meshReset = dynamic_cast<Button*>(FindNamedUI(meshField, "RevertProperty"));
	auto* meshValue = FindNamedUI(meshField, "PropertyValueArea");
	auto* scriptValue = FindNamedUI(script, "PropertyValueArea");
	auto* meshResetSlot = meshReset != nullptr ? dynamic_cast<Control*>(meshReset->GetParent()) : nullptr;
	check(positionReset != nullptr && positionReset->IsVisibleInHierarchy(),
		"modified position offers native default reset on initial selection");
	check(meshValue != nullptr && scriptValue != nullptr
		&& std::abs(meshValue->GetGlobalPosition().x - scriptValue->GetGlobalPosition().x) < 1.f,
		"Mesh and Script share the same value column");
	check(meshResetSlot != nullptr && meshValue != nullptr
		&& meshResetSlot->GetGlobalPosition().x + meshResetSlot->GetSize().x
			<= meshValue->GetGlobalPosition().x,
		"resource reset slot stays in the label column");

	if (positionReset != nullptr)
	{
		editor->GetUndoRedo().ClearHistory(scene->GetObjectID());
		click(positionReset);
		frame();
		check(mesh->GetLocalPosition() == Vector3::Zero && !positionReset->IsVisible(),
			"reset restores native position instead of the value at selection time");
		editor->GetUndoRedo().Undo(scene->GetObjectID());
		frame();
		check(mesh->GetLocalPosition() == Vector3(2.f, 3.f, 4.f), "property reset supports undo");
	}

	Ref<MeshResource> resource = CreateRef<MeshResource>();
	mesh->SetMeshResource(resource);
	frame();
	auto const capture = g_engine->m_fileSystem->GetResourceRoot().parent_path() / "Captures" / "inspector-resources.ppm";
	std::filesystem::create_directories(capture.parent_path());
	SaveRenderedFrame(g_engine->m_renderServer->ViewportGetTexture(
		static_cast<Viewport*>(tree.GetRoot())->GetViewportRID()), capture);
	if (meshReset != nullptr)
	{
		check(meshReset->IsVisibleInHierarchy(), "assigned Mesh offers reset to an empty resource");
		click(meshReset);
		frame();
		check(!mesh->GetMeshResource().IsValid(), "Mesh reset clears the resource through its setter");
		editor->GetUndoRedo().Undo(scene->GetObjectID());
		frame();
		check(mesh->GetMeshResource() == resource, "Mesh reset undo retains the previous resource");
	}

	group->SetVisible(false);
	frame();
	check(mesh->IsVisible() && !mesh->IsVisibleInHierarchy(), "hidden parent preserves the child's local visible flag");
	bool dimmed = false;
	for (Node* child : editor->m_editorUI->GetSceneTreeControl()->GetChildren())
	{
		if (auto* label = dynamic_cast<Label*>(child); label != nullptr && label->GetText() == "VisibilityMesh")
		{
			dimmed = label->GetFontColor() == editor->GetEditorTheme()->GetColor("guide_color", "Tree");
		}
	}

	check(dimmed, "a child hidden by its parent is dimmed in the Scene tree");
	group->SetVisible(true);
	mesh->SetVisible(false);
	group->SetVisible(false);
	group->SetVisible(true);
	check(!mesh->IsVisibleInHierarchy(), "showing the parent preserves a locally hidden child");
	mesh->SetVisible(true);

	Tree* sceneTree = editor->m_editorUI->GetSceneTreeControl();
	auto itemKey = [](Node* node)
	{
		return ((static_cast<uint64_t>(node->GetObjectID().GetUID()) << 32) | node->GetObjectID().GetIndex()) + 1;
	};
	frame();
	AABB2 const eye = sceneTree->GetItemActionRect(itemKey(group), 0);
	check(eye.GetWidth() > 0.f && sceneTree->GetItemActionRect(itemKey(plain), 0).GetWidth() == 0.f,
		"Node3D has an eye action while plain Node has none");
	if (eye.GetWidth() > 0.f)
	{
		InputEvent event;
		event.m_type = InputEventType::MouseButton;
		event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
		event.m_position = sceneTree->GetGlobalPosition() + eye.GetCenter();
		event.m_pressed = true;
		sceneTree->GetViewport()->ProcessInputEvent(event);
		event.m_pressed = false;
		sceneTree->GetViewport()->ProcessInputEvent(event);
		frame();
		check(!group->IsVisible() && !mesh->IsVisibleInHierarchy(), "Scene eye hides a complete Node3D branch");
		editor->GetUndoRedo().Undo(scene->GetObjectID());
		frame();
		check(group->IsVisible() && mesh->IsVisibleInHierarchy(), "Scene eye undo restores effective visibility");
	}

	group->SetVisible(false);
	mesh->Reparent(scene);
	check(mesh->IsVisibleInHierarchy(), "reparenting out of a hidden group restores visibility");
	mesh->Reparent(group);
	check(!mesh->IsVisibleInHierarchy(), "reparenting under a hidden group updates visibility");
	control->SetVisible(false);
	Ref<PackedScene> packed = CreateRef<PackedScene>();
	check(packed->Pack(scene), "visibility fixture packs successfully");
	Node* restored = packed->Instantiate();
	auto* restoredGroup = restored != nullptr ? dynamic_cast<Node3D*>(restored->FindChildByName("HiddenGroup")) : nullptr;
	auto* restoredMesh = restored != nullptr ? dynamic_cast<Node3D*>(restored->FindChildByName("VisibilityMesh")) : nullptr;
	auto* restoredControl = restored != nullptr ? dynamic_cast<CanvasItem*>(restored->FindChildByName("VisibleControl")) : nullptr;
	check(restoredGroup != nullptr && !restoredGroup->IsVisible() && restoredMesh != nullptr
		&& restoredMesh->IsVisible() && !restoredMesh->IsVisibleInHierarchy()
		&& restoredControl != nullptr && !restoredControl->IsVisible(),
		"scene serialization preserves local 3D and CanvasItem visibility");
	MemDelete(restored);

	// 1) Render a separate minimal 3D viewport to verify the visibility publication boundary
	auto* viewport = MemNew<SubViewport>();
	tree.GetRoot()->AddNode(viewport);
	viewport->UpdateResolution(IntVec2(64, 64));
	auto* camera = MemNew<Camera3D>();
	viewport->AddNode(camera);
	camera->SetCurrent();
	auto* renderGroup = MemNew<Node3D>();
	viewport->AddNode(renderGroup);
	auto* cube = MemNew<MeshInstance3D>();
	cube->SetLocalPosition(Vector3(3.f, 0.f, 0.f));
	renderGroup->AddNode(cube);
	std::vector<Vertex> vertices;
	AddVertsForAABB3D(vertices, AABB3(Vector3(-0.7f, -0.7f, -0.7f), Vector3(0.7f, 0.7f, 0.7f)), Color::White);
	SurfaceTool surface;
	for (Vertex const& vertex : vertices)
	{
		surface.SetColor(vertex.m_color);
		surface.AddVertex(vertex.m_position);
	}

	surface.GenerateNormals();
	surface.Index();
	Ref<MeshResource> renderMesh = surface.CreateMeshResource();
	Ref<MaterialResource> material = CreateRef<MaterialResource>();
	material->SetShaderResource(g_engine->m_renderServer->GetDefaultUnlitShaderResource());
	material->SetTint(Color::Red);
	renderMesh->SetMaterial(material);
	cube->SetMeshResource(renderMesh);
	auto pixel = [&]()
	{
		frame();
		return ReadPixel(g_engine->m_renderServer->ViewportGetTexture(viewport->GetViewportRID()), 32, 32);
	};
	Color const visible = pixel();
	check(visible.r > 200 && visible.g < 30, "visible 3D fixture draws its mesh");
	renderGroup->SetVisible(false);
	Color const hidden = pixel();
	check(hidden != visible && cube->IsVisible(), "hiding the parent removes the mesh from GPU output");
	renderGroup->SetVisible(true);
	check(pixel() == visible, "showing the parent restores GPU output");
	renderGroup->SetVisible(false);
	cube->Reparent(viewport);
	check(pixel() == visible, "reparenting publishes effective visibility to the renderer");
	viewport->QueueFree();
	tree.FlushPendingNode();
}
