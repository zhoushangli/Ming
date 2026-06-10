#pragma once

#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include "MingEngine/Engine/Math/Vec2.hpp"

class Camera3D;
class EditorUI;
class EditorGizmos;

class EditorSelection
{
public:
	NodeHandle GetSelected() const;
	void SetSelected(NodeHandle handle);
	void Clear();

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

	EditorSelection& GetSelection();
	EditorSelection const& GetSelection() const;

	void SetActiveCamera(Camera3D* camera);

	// --- Mouse event routing ---
	void OnMouseMove(Vec2 screenPos, Vec2 delta);
	void OnMouseDown(int keyCode, Vec2 screenPos);
	void OnMouseUp(int keyCode, Vec2 screenPos);

private:
	void OnProcess(float deltaSeconds) override;

	void SaveSceneToFile(Node const* sceneRoot, std::string const& filename);

private:
	EditorSelection m_selection;
	EditorGizmos* m_editorGizmos = nullptr;
	EditorUI* m_editorUI         = nullptr;
	Camera3D* m_activeCamera     = nullptr;

	static EditorNode* s_instance;
};
