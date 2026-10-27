#include "MingEngine/Editor/UI/EditorControlHost.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/JetBrainsMonoFont.hpp"

#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"
#include "ThirdParty/imgui/imgui.h"

#include <algorithm>

EditorControlHost::~EditorControlHost()
{
	Interrupt();
	if (Viewport* viewport = GetViewport())
	{
		if (!m_native)
		{
			viewport->QueueFree();
		}
		else if (m_root != nullptr && !m_root->IsQueuedForDeletion())
		{
			m_root->QueueFree();
		}
	}
}

Viewport* EditorControlHost::GetViewport() const
{
	return ObjectDatabase::GetInstance<Viewport>(m_viewportID);
}

void EditorControlHost::Initialize(SceneTree* tree, bool scrolling)
{
	if (GetViewport() != nullptr || tree == nullptr)
	{
		return;
	}

	m_native           = g_engine->m_imguiSystem == nullptr;
	EditorNode* editor = EditorNode::Get();
	Node* parent = editor != nullptr && editor->GetSceneTree() == tree ? static_cast<Node*>(editor) : tree->GetRoot();
	Viewport* viewport = nullptr;
	if (m_native)
	{
		viewport = static_cast<Viewport*>(tree->GetRoot());
	}
	else
	{
		auto* subViewport = MemNew<SubViewport>();
		subViewport->SetSerializable(false);
		parent->AddNode(subViewport);
		viewport = subViewport;
		parent   = viewport;
	}

	m_viewportID = viewport->GetObjectID();
	m_content    = MemNew<VBoxContainer>();
	m_content->SetSeparation(8.f);
	if (scrolling)
	{
		auto* scroll = MemNew<ScrollContainer>();
		parent->AddNode(scroll);
		scroll->AddNode(m_content);
		m_root = scroll;
	}
	else
	{
		parent->AddNode(m_content);
		m_root = m_content;
	}

	m_root->SetSerializable(false);
	m_font = JetBrainsMonoFont::CreateNativeFont();
}

void EditorControlHost::ApplyFont(Node* node)
{
	if (auto* label = dynamic_cast<Label*>(node))
	{
		label->SetFont(m_font);
	}
	else if (auto* edit = dynamic_cast<LineEdit*>(node))
	{
		edit->SetFont(m_font);
	}
	else if (auto* button = dynamic_cast<Button*>(node))
	{
		button->SetFont(m_font);
	}

	for (Node* child : node->GetChildren())
	{
		ApplyFont(child);
	}
}

void EditorControlHost::Draw(Vector2 size)
{
	if (m_native)
	{
		DrawNative(m_origin, size);
		return;
	}

	Viewport* viewport = GetViewport();
	if (viewport == nullptr)
	{
		return;
	}

	m_drawn             = true;
	m_visible           = true;
	ImVec2 const origin = ImGui::GetCursorScreenPos();
	m_origin            = Vector2(origin.x, origin.y);
	m_size              = Vector2(std::max(1.f, size.x), std::max(1.f, size.y));
	IntVec2 const resolution(static_cast<int>(m_size.x), static_cast<int>(m_size.y));
	if (viewport->GetOutputResolution() != resolution)
	{
		if (auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup()))
		{
			popup->Close();
		}

		viewport->UpdateResolution(resolution);
	}

	ApplyFont(m_root);
	m_root->UpdateSize(m_size);
	viewport->GetSceneTree()->FlushContainerUpdate();
	GPUTexture* texture = g_engine->m_renderServer->ViewportGetTexture(viewport->GetViewportRID());
	if (texture != nullptr)
	{
		ImGui::Image(ImTextureRef(g_engine->m_renderServer->GetImGuiTextureID(texture)), ImVec2(m_size.x, m_size.y));
	}
	else
	{
		ImGui::Dummy(ImVec2(m_size.x, m_size.y));
	}
}

void EditorControlHost::EndFrame()
{
	if (!m_drawn)
	{
		Interrupt();
		m_visible = false;
	}
}

bool EditorControlHost::Contains(Vector2 position) const
{
	Vector2 const local = position - m_origin;
	return m_visible && local.x >= 0.f && local.y >= 0.f && local.x < m_size.x && local.y < m_size.y;
}

Vector2 EditorControlHost::ToLocal(Vector2 position) const
{
	Viewport*     viewport   = GetViewport();
	Vector2 const resolution = viewport != nullptr ? Vector2(viewport->GetOutputResolution()) : m_size;
	Vector2 const local      = position - m_origin;
	return Vector2(local.x * resolution.x / std::max(1.f, m_size.x), local.y * resolution.y / std::max(1.f, m_size.y));
}

bool EditorControlHost::HasKeyboardFocus() const
{
	Viewport* viewport = GetViewport();
	if (m_native && viewport != nullptr)
	{
		for (Node* node = viewport->GetGuiFocus(); node != nullptr; node = node->GetParent())
		{
			if (node == m_root)
			{
				return true;
			}
		}

		return false;
	}

	return m_visible && m_active && viewport != nullptr && viewport->GetGuiFocus() != nullptr;
}

bool EditorControlHost::HasMouseCapture() const
{
	Viewport* viewport = GetViewport();
	return m_visible && viewport != nullptr && viewport->HasGuiMouseCapture();
}

void EditorControlHost::Interrupt()
{
	m_outsideClick = false;
	m_active       = false;
	if (Viewport* viewport = GetViewport())
	{
		if (auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup()))
		{
			bool owned = !m_native;
			for (Node* node = popup->GetOwner(); node != nullptr && !owned; node = node->GetParent())
			{
				owned = node == m_root;
			}

			if (owned)
			{
				popup->Close(false);
			}
		}

		viewport->CancelGuiInteraction(m_native ? static_cast<Node*>(m_root) : viewport);
	}
}

void EditorControlHost::FinishOutsideClick()
{
	if (m_outsideClick)
	{
		m_outsideClick = false;
		if (Viewport* viewport = GetViewport())
		{
			if (auto* popup = dynamic_cast<PopupPanel*>(viewport->GetGuiPopup()))
			{
				popup->Close(false);
			}

			viewport->SetGuiFocus(nullptr);
		}

		m_active = false;
	}
}

bool EditorControlHost::HandleInput(InputEvent const& event)
{
	Viewport* viewport = GetViewport();
	if (viewport == nullptr || !m_visible || m_native)
	{
		return false;
	}

	if (event.m_type == InputEventType::WindowFocus || event.m_type == InputEventType::CursorEnter)
	{
		if (event.m_type == InputEventType::WindowFocus && !event.m_focused)
		{
			Interrupt();
		}

		viewport->ProcessInputEvent(event);
		return false;
	}

	bool const pointer = event.m_type == InputEventType::MouseButton || event.m_type == InputEventType::MouseMotion
						 || event.m_type == InputEventType::MouseWheel;
	if (pointer)
	{
		bool const inside = Contains(event.m_position);
		if (!inside && !viewport->HasGuiMouseCapture())
		{
			viewport->SetGuiHovered(nullptr);
			if (event.m_type == InputEventType::MouseButton && event.m_pressed)
			{
				m_outsideClick = true;
			}

			return false;
		}

		if (event.m_type == InputEventType::MouseButton && event.m_pressed)
		{
			m_active       = true;
			m_outsideClick = false;
		}

		InputEvent local = event;
		local.m_position = ToLocal(event.m_position);
		viewport->ProcessInputEvent(local);
		return true;
	}

	if ((event.m_type == InputEventType::Key || event.m_type == InputEventType::Text) && HasKeyboardFocus()
		&& !m_outsideClick)
	{
		viewport->ProcessInputEvent(event);
		return true;
	}

	return false;
}

void EditorControlHost::DrawNative(Vector2 origin, Vector2 size)
{
	if (m_root == nullptr)
	{
		return;
	}

	m_drawn            = true;
	m_visible          = true;
	m_origin           = origin;
	m_size             = size;
	CanvasItem* parent = m_root->GetParentItem();
	m_root->SetPosition(parent != nullptr ? origin - parent->GetGlobalPosition() : origin);
	m_root->UpdateSize(size);
	ApplyFont(m_root);
}
