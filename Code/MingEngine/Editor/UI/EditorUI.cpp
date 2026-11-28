#include "MingEngine/Scene/Resource/StandardMaterial2D.hpp"
#include "MingEngine/Scene/Resource/StandardMaterial3D.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/IO/ResourceSaver.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/Resource/ShaderMaterial.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "ThirdParty/GLFW/glfw3.h"

Control* EditorUI::GetRootControl() const
{
	return static_cast<Control*>(m_nativeRoot);
}

bool EditorUI::HandleInput(InputEvent const& event)
{
	if (event.m_type != InputEventType::MouseMotion && m_tooltip != nullptr)
	{
		m_tooltip->SetVisible(false);
		m_tooltipSource = ObjectID::Invalid;
	}

	if (m_owner == nullptr)
	{
		return false;
	}

	if (event.m_type == InputEventType::WindowFocus && !event.m_focused)
	{
		m_viewportPanel->CancelNavigation();
	}

	if ((m_owner->IsBuildingCSharpProject() || m_owner->IsPreparingPlay() || m_owner->IsCheckingProject())
		&& static_cast<Viewport*>(m_owner->GetSceneTree()->GetRoot())->GetGuiPopup() == nullptr)
	{
		return true;
	}

	UpdateNativeDocking();
	m_owner->GetSceneTree()->FlushContainerUpdate();
	auto* root = static_cast<Viewport*>(m_owner->GetSceneTree()->GetRoot());
	if (root->GetGuiPopup() == nullptr && event.m_type == InputEventType::Key && event.m_pressed
		&& (event.m_modifiers & GLFW_MOD_CONTROL) != 0)
	{
		if ((event.m_keyCode == GLFW_KEY_Z || event.m_keyCode == GLFW_KEY_Y)
			&& dynamic_cast<LineEdit*>(root->GetGuiFocus()) == nullptr)
		{
			if (ObjectID history = m_inspectorPanel->GetUndoHistory(); history.IsValid())
			{
				FinishNativeEditing();
				if (event.m_keyCode == GLFW_KEY_Y || (event.m_modifiers & GLFW_MOD_SHIFT) != 0)
				{
					m_owner->GetUndoRedo().Redo(history);
				}
				else
				{
					m_owner->GetUndoRedo().Undo(history);
				}
			}
			return true;
		}

		switch (event.m_keyCode)
		{
		case GLFW_KEY_N:
			m_owner->RequestNewScene();
			return true;
		case GLFW_KEY_O:
			m_owner->RequestOpenScene();
			return true;
		case GLFW_KEY_S:
			if ((event.m_modifiers & GLFW_MOD_SHIFT) != 0)
			{
				m_inspectorPanel->SaveResourceAs();
			}
			else
			{
				m_inspectorPanel->SaveResource();
			}
			return true;
		case GLFW_KEY_W:
			m_owner->RequestCloseScene();
			return true;
		}
	}

	if (event.m_type == InputEventType::Key && event.m_pressed && event.m_keyCode == GLFW_KEY_F5)
	{
		m_owner->CheckProject(true);
		return true;
	}
	HandleMenuPointer(event);
	root->ProcessInputEvent(event);
	UpdateNativeDocking();
	root->UpdateGuiDrag(root->GetGuiDragPosition());
	return true;
}

bool EditorUI::IsNativeInputActive() const
{
	if (m_owner == nullptr)
	{
		return true;
	}

	auto* root = static_cast<Viewport*>(m_owner->GetSceneTree()->GetRoot());
	return root->GetGuiPopup() != nullptr || (root->GetGuiFocus() != nullptr && root->GetGuiFocus() != m_sceneView)
		   || (!IsSceneNavigationActive() && !IsSceneHovered());
}

void EditorUI::FinishInput(EditorUIContext& context)
{
	m_inspectorPanel->FinishInput(context);
}

void EditorUI::Warning(std::string const& title, std::string const& message)
{
	m_warningPopup.Open(title, message);
	if (m_nativeRoot != nullptr)
	{
		EditorUIContext context;
		context.m_rootControl = GetRootControl();
		context.m_sceneTree   = m_owner->GetSceneTree();
		context.m_editorUI    = this;
		m_warningPopup.Render(context);
	}
}

void EditorUI::SetViewportRect(Vector2 origin, Vector2 dims)
{
	m_viewportOrigin = origin;
	m_viewportDims   = dims;
}

Vector2 EditorUI::GetViewportDimensions() const
{
	return m_sceneView != nullptr && m_sceneView->GetSubViewport() != nullptr
		? Vector2(m_sceneView->GetSubViewport()->GetOutputResolution()) : m_viewportDims;
}

Vector2 EditorUI::ToViewportPos(Vector2 position) const
{
	return m_sceneView != nullptr ? m_sceneView->ToViewportPosition(position) : position - m_viewportOrigin;
}

bool EditorUI::IsSceneHovered() const
{
	if (m_sceneView == nullptr || g_engine->m_inputSystem == nullptr)
	{
		return false;
	}

	Vector2 const position(g_engine->m_inputSystem->GetCursorClientPosition());
	return m_sceneView->GetViewport()->GetGuiPopup() == nullptr && !m_sceneView->GetViewport()->IsGuiDragging()
		   && m_sceneView->IsVisibleInHierarchy()
		   && m_sceneView->IsPointInside(position - m_sceneView->GetGlobalPosition());
}

bool EditorUI::IsSceneNavigationActive() const
{
	return m_viewportPanel != nullptr && m_viewportPanel->IsNavigating();
}

void EditorUI::TogglePanel(EditorPanel* panel)
{
	if (panel == m_viewportPanel)
	{
		return;
	}

	auto* slot = dynamic_cast<TabContainer*>(panel->GetParent());
	if (panel->IsOpen() && slot != nullptr)
	{
		slot->RemovePage(slot->GetPageIndex(panel));
		GetRootControl()->AddNode(panel);
		panel->Close();
	}
	else if (!panel->IsOpen())
	{
		int const destination = panel == m_inspectorPanel    ? 2
								: panel == m_outputPanel     ? 3
								: panel == m_fileSystemPanel ? 1
															 : 0;
		panel->Open();
		m_dockSlots[destination]->AddPage(panel->GetTitle(), panel);
	}

	UpdateNativeDocking();
	SaveNativeLayout();
}

void EditorUI::SaveMaterialAs(Ref<MaterialResource> const& source, std::function<void(VirtualPath const&)> created, VirtualPath const& directory)
{
	VirtualPath initial = source.IsValid() && source->GetVirtualPath().IsValid() ? source->GetVirtualPath().GetParent() : directory;
	OpenSceneFile(SceneFilePopup::Mode::ResourceSave, initial.Join(source.IsValid() ? source->GetClassName() + ".tres" : "ShaderMaterial.tres"),
		[this, source, created](VirtualPath const& path)
		{
			FinishNativeEditing();
			if (source.IsValid() && path == source->GetVirtualPath())
			{
				Warning("Save Material As", "Choose a different path to create an independent material.");
				return false;
			}

			Ref<MaterialResource> material = source.IsValid()
				? Ref<MaterialResource>(dynamic_cast<MaterialResource*>(ClassDatabase::CreateInstance(source->GetClassName())))
				: Ref<MaterialResource>(CreateRef<ShaderMaterial>());
			if (!material.IsValid())
			{
				return false;
			}
			if (g_engine->m_fileSystem->Exists(path))
			{
				Ref<Resource> existing = ResourceLoader::LoadUncached(path);
				if (existing.IsValid() && existing->GetClassName() != material->GetClassName())
				{
					Warning("Save Material As", "Choose a different path when saving a different resource type.");
					return false;
				}
			}
			if (source.IsValid())
			{
				material->CopyValuesFrom(*source);
			}

			material->SetName(path.GetStem());
			if (!ResourceSaver::Save(path, material))
			{
				return false;
			}

			ResourceLoader::ReloadIfCached(path);
			if (created)
			{
				created(path);
			}

			return true;
		});
}

void EditorUI::CreateResource(VirtualPath const& directory, std::function<void(VirtualPath const&)> created)
{
	FinishNativeEditing();
	if (m_createResourcePopup == nullptr)
	{
		m_createResourcePopup = EditorPopupUtils::Create(GetRootControl(), "Create Resource", m_createResourceContent);
		m_createResourcePopup->SetName("CreateResourcePopup");
	}

	EditorPopupUtils::ClearContent(m_createResourceContent);
	EditorPopupUtils::AddLabel(m_createResourceContent, "Resource type:");
	auto* types = MemNew<Tree>();
	types->SetName("ResourceTypes");
	types->SetVerticalSizeFlags(Control::Size_ExpandFill);
	types->SetCustomMinimumSize(Vector2(0.f, 120.f));
	m_createResourceContent->AddNode(types);
	types->SetItems({ { 1, 0, "ShaderMaterial", true, false,
		EditorIcons::GetIconTexture("ShaderMaterial"), EditorIcons::GetIconColor("ShaderMaterial", types) },
		{ 2, 0, "StandardMaterial2D", true, false, EditorIcons::GetIconTexture("StandardMaterial3D"), EditorIcons::GetIconColor("StandardMaterial3D", types) },
		{ 3, 0, "StandardMaterial3D", true, false, EditorIcons::GetIconTexture("StandardMaterial3D"), EditorIcons::GetIconColor("StandardMaterial3D", types) } });
	types->Select(1);
	EditorPopupUtils::AddLabel(m_createResourceContent, "A material drawn with a shader.", "EditorStatusLabel");
	auto create = [this, directory, created, types]()
	{
		Ref<MaterialResource> material;
		switch (types->GetSelected())
		{
		case 2:
			material = CreateRef<StandardMaterial2D>();
			break;
		case 3:
			material = CreateRef<StandardMaterial3D>();
			break;
		default:
			material = CreateRef<ShaderMaterial>();
			break;
		}
		m_createResourcePopup->Close();
		SaveMaterialAs(material, [this, created](VirtualPath const& path)
		{
			g_engine->m_fileSystem->ScanResourceTree();
			m_fileSystemPanel->NavigateTo(path.GetParent());
			m_fileSystemPanel->SelectPath(path);
			if (created)
			{
				created(path);
			}
		}, directory);
	};
	types->SetActivatedCallback([create](uint64_t) { create(); });
	auto* actions = EditorPopupUtils::AddActions(m_createResourceContent);
	actions->SetSeparation(static_cast<float>(GetRootControl()->GetThemeConstant("spacing", "Editor")));
	EditorPopupUtils::AddButton(actions, "Cancel", [this]() { m_createResourcePopup->Close(); });
	Button* confirm = EditorPopupUtils::AddButton(actions, "Create", create);
	confirm->SetThemeTypeVariation("EditorPrimaryButton");
	EditorPopupUtils::Open(m_createResourcePopup, GetRootControl(),
		Vector2(static_cast<float>(GetRootControl()->GetThemeConstant("project_popup_width", "Editor")), 0.f));
	types->GrabFocus();
}
