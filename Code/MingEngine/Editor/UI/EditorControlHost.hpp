#pragma once

#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Engine/Input/InputEvent.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"

class SceneTree;
class Node;
class SubViewport;
class Viewport;
class Control;
class VBoxContainer;

class EditorControlHost
{
public:
	~EditorControlHost();
	void           Initialize(SceneTree* tree, bool scrolling = true);
	VBoxContainer* GetContent() const { return m_content; }
	Viewport*      GetViewport() const;
	void           DrawNative(Vector2 origin, Vector2 size);
	Control*       GetRootControl() const { return m_root; }
	void           SetFont(Ref<FontResource> const& font) { m_font = font; }
	void           BeginFrame() { m_drawn = false; }
	void           EndFrame();
	void           Draw(Vector2 size);
	bool           HandleInput(InputEvent const& event);
	void           FinishOutsideClick();
	void           Interrupt();
	bool           HasKeyboardFocus() const;
	bool           HasMouseCapture() const;
	bool           Contains(Vector2 position) const;
	Vector2        ToLocal(Vector2 position) const;
	Vector2        GetOrigin() const { return m_origin; }

private:
	void              ApplyFont(Node* node);
	ObjectID          m_viewportID = ObjectID::Invalid;
	VBoxContainer*    m_content    = nullptr;
	Control*          m_root       = nullptr;
	Ref<FontResource> m_font;
	Vector2           m_origin       = Vector2::Zero;
	Vector2           m_size         = Vector2::Zero;
	bool              m_visible      = false;
	bool              m_drawn        = false;
	bool              m_active       = false;
	bool              m_outsideClick = false;
	bool              m_native       = false;
};
