#pragma once

template <typename Check, typename Frame>
void VerifyEditorNumericAndGizmos(EditorNode* editor, Viewport* root, Check check, Frame frame)
{
	auto send = [&](InputEventType type,
					Vector2        position,
					bool           pressed   = false,
					Vector2        relative  = Vector2::Zero,
					int            modifiers = 0,
					int            button    = GLFW_MOUSE_BUTTON_LEFT)
	{
		InputEvent event;
		event.m_type        = type;
		event.m_position    = position;
		event.m_relative    = relative;
		event.m_pressed     = pressed;
		event.m_modifiers   = modifiers;
		event.m_mouseButton = button;
		root->ProcessInputEvent(event);
	};
	auto key = [&](int code, int modifiers = 0)
	{
		InputEvent event;
		event.m_type      = InputEventType::Key;
		event.m_keyCode   = code;
		event.m_modifiers = modifiers;
		event.m_pressed   = true;
		root->ProcessInputEvent(event);
	};
	auto type = [&](char const* text)
	{
		key(GLFW_KEY_A, GLFW_MOD_CONTROL);
		for (char const* p = text; *p != 0; ++p)
		{
			InputEvent event;
			event.m_type      = InputEventType::Text;
			event.m_codepoint = *p;
			root->ProcessInputEvent(event);
		}
	};

	// 1) Exercise the numeric control through real Viewport focus and capture dispatch.
	auto* numeric = MemNew<EditorSpinSlider>();
	numeric->SetName("NumericInteractionFixture");
	numeric->SetThemeSource(editor->m_editorUI->GetRootControl());
	numeric->SetPosition(Vector2(25.f, 220.f));
	numeric->Configure(false, "-10,10,0.1");
	numeric->SetValueNoSignal(Variant(2.f));
	root->AddNode(numeric);
	numeric->UpdateSize(Vector2(180.f, numeric->GetCombinedMinimumSize().y));
	float model    = 2.f;
	float original = model;
	int   commits  = 0;
	numeric->SetValueReader(
		[&]()
		{
			return Variant(model);
		});
	numeric->SetValueChangedCallback(
		[&](Variant const& value)
		{
			model = value.As<float>();
		});
	numeric->SetDragCallbacks(
		[&]()
		{
			original = model;
		},
		[&](Variant const& value)
		{
			model = value.As<float>();
		},
		[&](bool accept)
		{
			if (accept && model != original)
			{
				++commits;
			}
			else if (!accept)
			{
				model = original;
			}
		});
	frame();
	Vector2 at = numeric->GetGlobalPosition() + numeric->GetSize() * 0.5f;
	send(InputEventType::MouseButton, at, true);
	send(InputEventType::MouseButton, at);
	check(
		numeric->GetLineEdit()->HasFocus() && numeric->GetLineEdit()->GetSelectedText() == String("2"),
		"numeric click enters text editing and selects the complete value");
	send(InputEventType::MouseButton, at, true);
	send(InputEventType::MouseButton, at);
	check(
		!numeric->GetLineEdit()->HasSelection(),
		"second numeric click positions the caret instead of selecting all again");
	type("-3.5");
	key(GLFW_KEY_ENTER);
	check(model == -3.5f && !numeric->GetLineEdit()->IsEditing(), "numeric Enter commits a typed finite value");
	numeric->GrabFocus();
	frame();
	key(GLFW_KEY_ENTER);
	check(numeric->GetLineEdit()->GetSelectedText() == String("-3.5"), "numeric keyboard entry selects all");
	type("-");
	key(GLFW_KEY_ENTER);
	check(model == -3.5f && !numeric->GetError().empty(), "incomplete numeric input never publishes zero");
	send(InputEventType::MouseButton, at, true);
	send(InputEventType::MouseButton, at);
	type("9");
	key(GLFW_KEY_ESCAPE);
	check(model == -3.5f, "numeric text Escape keeps the original property value");
	root->SetGuiFocus(nullptr);
	frame();

	send(InputEventType::MouseButton, at, true);
	send(InputEventType::MouseMotion, at + Vector2(2.f, 0.f), false, Vector2(2.f, 0.f));
	check(model == -3.5f && !numeric->IsDragging(), "subthreshold mouse movement does not change numeric values");
	send(InputEventType::MouseMotion, at + Vector2(22.f, 0.f), false, Vector2(20.f, 0.f));
	check(model > -3.5f && numeric->IsDragging(), "horizontal numeric dragging previews values");
	float beforeFine = model;
	send(InputEventType::MouseMotion, at + Vector2(42.f, 0.f), false, Vector2(20.f, 0.f), GLFW_MOD_SHIFT);
	check(model >= beforeFine && model - beforeFine < 0.5f, "Shift gives precise numeric drag changes");
	send(InputEventType::MouseMotion, at + Vector2(62.f, 0.f), false, Vector2(20.f, 0.f), GLFW_MOD_CONTROL);
	check(model == std::round(model), "Ctrl rounds floating point drags to integers");
	send(InputEventType::MouseButton, Vector2(-20.f, 220.f));
	check(
		!numeric->IsDragging() && commits == 1 && !root->HasGuiMouseCapture(),
		"release outside the control finishes one numeric edit and releases capture");
	float const accepted = model;
	send(InputEventType::MouseButton, at, true);
	send(InputEventType::MouseMotion, at - Vector2(20.f, 0.f), false, Vector2(-20.f, 0.f));
	key(GLFW_KEY_ESCAPE);
	send(InputEventType::MouseButton, at);
	check(
		model == accepted && commits == 1 && !numeric->IsDragging(),
		"numeric drag Escape restores the initial value");
	send(InputEventType::MouseButton, at, true);
	send(InputEventType::MouseMotion, at + Vector2(10000.f, 0.f), false, Vector2(10000.f, 0.f));
	check(model == 10.f, "numeric dragging obeys configured bounds");
	send(InputEventType::MouseButton, at, true, Vector2::Zero, 0, GLFW_MOUSE_BUTTON_RIGHT);
	send(InputEventType::MouseButton, at);
	check(model == accepted, "right click cancels numeric dragging");
	root->SetGuiFocus(nullptr);
	send(InputEventType::MouseButton, at, true);
	send(InputEventType::MouseMotion, at + Vector2(20.f, 0.f), false, Vector2(20.f, 0.f));
	InputEvent focus;
	focus.m_type = InputEventType::WindowFocus;
	root->ProcessInputEvent(focus);
	check(
		model == accepted && !numeric->IsDragging() && !root->HasGuiMouseCapture(),
		"window focus loss cancels numeric preview and releases capture");
	focus.m_focused = true;
	root->ProcessInputEvent(focus);

	auto* nextText = MemNew<LineEdit>();
	nextText->SetText("plain text");
	nextText->SetPosition(Vector2(25.f, 280.f));
	root->AddNode(nextText);
	nextText->UpdateSize(Vector2(180.f, nextText->GetCombinedMinimumSize().y));
	frame();
	nextText->GrabFocus();
	key(GLFW_KEY_TAB, GLFW_MOD_SHIFT);
	frame();
	check(
		numeric->GetLineEdit()->HasFocus() && numeric->GetLineEdit()->HasSelection(),
		"Tab navigation enters numeric text editing with all text selected");
	key(GLFW_KEY_TAB);
	check(nextText->HasFocus() && !nextText->HasSelection(), "ordinary text fields retain their normal focus behavior");
	nextText->QueueFree();
	root->SetGuiFocus(nullptr);
	numeric->EndEditing();
	numeric->SetDragCallbacks({}, {}, {});
	numeric->SetValueReader({});
	numeric->SetValueChangedCallback({});
	numeric->QueueFree();
	frame();

	// 2) Each node fixture supplies its own shape and editable properties.
	SceneTree* tree        = root->GetSceneTree();
	Node*      scene       = tree->GetScene();
	auto*      omni        = MemNew<OmniLight3D>();
	auto*      spot        = MemNew<SpotLight3D>();
	auto*      directional = MemNew<DirectionalLight3D>();
	auto*      camera      = MemNew<Camera3D>();
	omni->SetRange(1.5f);
	spot->SetRange(2.f);
	Node3D* targets[] = { omni, spot, directional, camera };
	for (int i = 0; i < 4; ++i)
	{
		targets[i]->SetName(Stringf("NodeGizmoFixture%d", i));
		targets[i]->SetLocalPosition(Vector3(9.f, static_cast<float>(i * 3 - 4), 0.f));
		scene->AddNode(targets[i]);
	}
	auto* viewCamera = editor->GetEditorCamera()->GetCamera();
	editor->GetEditorCamera()->SetLocalPosition(Vector3::Zero);
	editor->GetEditorCamera()->SetLocalOrientation(EulerAngles::Zero);
	viewCamera->SetPerspective(60.f);
	viewCamera->SetCurrent();
	auto* gizmos = editor->m_editorGizmos;
	auto* view   = editor->m_editorUI->GetPanel<ViewportPanel>().GetSceneView();
	auto  screen = [&](Vector3 const& world)
	{
		Vector3 const point   = viewCamera->GetWorldInverseTransform().TransformPosition3D(world);
		float const   tangent = std::tan(viewCamera->GetFovDegrees() * 3.14159265f / 360.f);
		return view->GetGlobalPosition() + view->GetSize() * 0.5f
			   - Vector2(point.y, point.z) * (view->GetSize().y / (2.f * point.x * tangent));
	};
	auto find = [&](Node3D* target) -> EditorNodeGizmo3D*
	{
		for (Node* child : gizmos->GetChildren())
		{
			auto* gizmo = dynamic_cast<EditorNodeGizmo3D*>(child);
			if (gizmo != nullptr && !gizmo->IsQueuedForDeletion() && gizmo->GetTargetID() == target->GetObjectID())
			{
				return gizmo;
			}
		}
		return nullptr;
	};
	frame();
	frame();
	for (Node3D* target : targets)
	{
		editor->GetSelection().SetSelected(target->GetObjectID());
		frame();
		auto* gizmo = find(target);
		check(
			gizmo != nullptr && !gizmo->GetSerializable(),
			"supported nodes have editor-owned nonserializable gizmos");
		check(
			gizmos->PickNode(*viewCamera, screen(target->GetWorldPosition()), 10000.f) == target->GetObjectID(),
			"node gizmo icons are selectable in the viewport");
	}
	check(
		find(directional)->GetHandlePositions().empty(),
		"directional lights use Transform instead of a range handle");
	auto drag = [&](Node3D* target, int handle, Vector3 const& destination)
	{
		editor->GetSelection().SetSelected(target->GetObjectID());
		frame();
		auto* gizmo = find(target);
		if (gizmo == nullptr || static_cast<size_t>(handle) >= gizmo->GetHandlePositions().size())
		{
			check(false, "node exposes the requested property handle");
			return false;
		}
		bool const started = gizmos->BeginDragHovered(*viewCamera, screen(gizmo->GetHandlePositions()[handle]));
		check(started && gizmo->IsDragging(), "node property handles take priority over Transform");
		gizmos->OnDrag(*viewCamera, screen(destination));
		return started && gizmo->IsDragging();
	};
	float const originalRange = omni->GetRange();
	drag(omni, 0, omni->GetWorldPosition() - Vector3::Left * 2.5f);
	check(std::abs(omni->GetRange() - 2.5f) < 0.02f, "point light handle edits world-space Range");
	gizmos->OnEndDrag();
	editor->GetUndoRedo().Undo(scene->GetObjectID());
	check(omni->GetRange() == originalRange, "point light drag is undone in one operation");
	editor->GetUndoRedo().Redo(scene->GetObjectID());
	check(std::abs(omni->GetRange() - 2.5f) < 0.02f, "point light drag can be redone");
	drag(omni, 0, omni->GetWorldPosition() - Vector3::Left * 4.f);
	gizmos->CancelDrag();
	check(std::abs(omni->GetRange() - 2.5f) < 0.02f, "canceling a light handle restores its original value");
	drag(spot, 0, spot->GetWorldPosition() + Vector3::Forward * 3.f);
	check(std::abs(spot->GetRange() - 3.f) < 0.02f, "spot light range handle uses the emission axis");
	gizmos->OnEndDrag();
	float const radians = 30.f * 3.14159265f / 180.f;
	drag(spot, 1, spot->GetWorldPosition() + Vector3(3.f * std::cos(radians), 3.f * std::sin(radians), 0.f));
	check(std::abs(spot->GetSpotAngle() - 30.f) < 0.1f, "spot angle handle works even when its plane is edge-on");
	gizmos->OnEndDrag();
	drag(camera, 0, camera->GetWorldPosition() + Vector3(1.f, 0.f, 1.f));
	check(std::abs(camera->GetFovDegrees() - 90.f) < 0.1f, "perspective camera handle edits vertical FOV");
	gizmos->OnEndDrag();
	camera->SetOrthogonal(2.f);
	drag(camera, 0, camera->GetWorldPosition() + Vector3(1.f, 0.f, 2.f));
	check(std::abs(camera->GetSize() - 4.f) < 0.02f, "orthographic camera handle edits vertical Size");
	gizmos->OnEndDrag();

	drag(camera, 0, camera->GetWorldPosition() + Vector3(1.f, 0.f, 9000.f));
	check(camera->GetSize() == 16384.f, "orthographic handle clamps to its maximum valid size");
	gizmos->CancelDrag();
	camera->SetPerspective(60.f);
	drag(camera, 0, camera->GetWorldPosition() + Vector3(1.f, 0.f, 1000.f));
	check(camera->GetFovDegrees() == 179.f, "perspective handle clamps its FOV below 180 degrees");
	gizmos->CancelDrag();
	frame();
	float maxWidth = 0.f;
	for (Vector3 const& endpoint : find(camera)->GetOutlineSegments())
	{
		maxWidth = std::max(maxWidth, std::abs(endpoint.y - camera->GetWorldPosition().y));
	}
	IntVec2 const output        = camera->GetViewport()->GetOutputResolution();
	float const   expectedWidth = std::tan(60.f * 3.14159265f / 360.f) * output.x / output.y;
	check(std::abs(maxWidth - expectedWidth) < 0.01f, "camera outline uses the camera viewport aspect ratio");

	auto* rotatedParent = MemNew<Node3D>();
	rotatedParent->SetLocalOrientation(EulerAngles(35.f, 15.f, 10.f));
	rotatedParent->SetLocalScale(Vector3(2.f, 3.f, 4.f));
	scene->AddNode(rotatedParent);
	auto* rotatedChild = MemNew<Node3D>();
	rotatedChild->SetLocalPosition(Vector3(4.f, 0.f, 0.f));
	rotatedChild->SetLocalOrientation(EulerAngles(20.f, 10.f, 5.f));
	rotatedParent->AddNode(rotatedChild);
	editor->GetSelection().SetSelected(rotatedChild->GetObjectID());
	gizmos->SetLocal(true);
	frame();
	GizmoContext const local = BuildGizmoContext(tree, *viewCamera);
	check(
		(local.m_basis.GetIBasis3D() - rotatedChild->GetWorldTransform().GetIBasis3D().GetNormalized()).GetLength()
				< 0.001f
			&& std::abs(local.m_basis.GetJBasis3D().GetLength() - 1.f) < 0.001f,
		"Local handles follow parent rotation while excluding inherited nonuniform scale");
	gizmos->SetLocal(false);
	check(
		BuildGizmoContext(tree, *viewCamera).m_basis.GetIBasis3D() == Vector3::Forward,
		"World mode retains fixed world axes after editing a parented node");
	gizmos->SetLocal(true);
	rotatedParent->QueueFree();

	// 3) Inspector and Gizmo publish the same property and share scene history.
	editor->GetSelection().SetSelected(omni->GetObjectID());
	frame();
	auto& inspector    = editor->m_editorUI->GetPanel<InspectorPanel>();
	auto* rangeControl = FindNamedUI(&inspector, "range");
	auto* field        = rangeControl != nullptr ? FindUIControl<EditorSpinSlider>(rangeControl) : nullptr;
	check(field != nullptr, "light Range uses the editor-specific numeric control");
	if (field != nullptr)
	{
		float const   before   = omni->GetRange();
		Vector2 const position = field->GetGlobalPosition() + field->GetSize() * 0.5f;
		send(InputEventType::MouseButton, position, true);
		send(InputEventType::MouseMotion, position + Vector2(20.f, 0.f), false, Vector2(20.f, 0.f));
		send(InputEventType::MouseButton, position + Vector2(20.f, 0.f));
		check(omni->GetRange() > before, "Inspector numeric drag writes through the light setter");
		frame();
		check(
			std::abs((find(omni)->GetHandlePositions()[0] - omni->GetWorldPosition()).GetLength() - omni->GetRange())
				< 0.02f,
			"Inspector Range changes refresh the Gizmo radius");
		editor->GetUndoRedo().Undo(scene->GetObjectID());
		check(std::abs(omni->GetRange() - before) < 0.02f, "Inspector drag creates one complete undo action");
		drag(omni, 0, omni->GetWorldPosition() - Vector3::Left * 3.5f);
		frame();
		check(
			std::abs(field->GetValue().As<float>() - omni->GetRange()) < 0.02f,
			"Gizmo Range changes refresh the Inspector without recursive writeback");
		omni->SetRange(7.f);
		gizmos->CancelDrag();
		check(omni->GetRange() == 7.f, "canceling a drag does not overwrite an external property change");
	}
	drag(spot, 0, spot->GetWorldPosition() + Vector3::Forward * 4.f);
	editor->GetSelection().SetSelected(camera->GetObjectID());
	frame();
	check(
		!gizmos->IsDragging() && std::abs(spot->GetRange() - 3.f) < 0.02f,
		"switching selection cancels an in-progress node handle drag");
	drag(omni, 0, omni->GetWorldPosition() - Vector3::Left * 4.f);
	ObjectID const deleted = find(omni)->GetObjectID();
	omni->QueueFree();
	frame();
	frame();
	check(
		!gizmos->IsDragging() && ObjectDatabase::GetInstance<Node>(deleted) == nullptr,
		"deleting an active target reclaims its node gizmo and drag state");
	editor->GetSelection().Clear();
	for (Node3D* target :
		 { static_cast<Node3D*>(spot), static_cast<Node3D*>(directional), static_cast<Node3D*>(camera) })
	{
		target->QueueFree();
	}
	frame();
	frame();
	check(!gizmos->IsDragging(), "deleting Gizmo targets leaves no active drag");
}
