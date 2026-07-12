#pragma once

#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Editor/EditorData.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

class Camera3D;
class EditorCamera;
class EditorUI;
class EditorGizmos;

class EditorSelection
{
public:
	NodeHandle GetSelected() const;
	void       SetSelected(NodeHandle handle);
	void       Clear();

private:
	NodeHandle m_selectedNodeHandle = NodeHandle::Invalid;
};

class EditorNode : public Node
{
	MCLASS(EditorNode, Node);

public:
	EditorNode();
	~EditorNode() override;

	static EditorNode* Get();

	EditorSelection&       GetSelection();
	EditorSelection const& GetSelection() const;

	EditorCamera* GetEditorCamera() const { return m_editorCamera; }

	bool LoadScene(std::string const& virtualPath);
	bool SaveScene(std::string const& virtualPath);

	// --- Mouse event routing ---
	void OnMouseMove(Vec2 screenPos, Vec2 delta);
	void OnMouseDown(int keyCode, Vec2 screenPos);
	void OnMouseUp(int keyCode, Vec2 screenPos);

private:
	void OnReady() override;
	void OnProcess(float deltaSeconds) override;

public:
	EditorSelection m_selection;
	EditorGizmos*   m_editorGizmos = nullptr;
	EditorCamera*   m_editorCamera = nullptr;
	EditorUI*       m_editorUI     = nullptr;

private:
	static EditorNode* s_instance;

	EditorData      m_editorData;
	EditorUIContext m_uiContext;
};
