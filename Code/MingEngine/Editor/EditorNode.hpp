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

	// Request a scene load and defer it when the current scene has unsaved changes.
	// e.g. RequestLoadScene("res://Scenes/Main.tscn")
	void RequestLoadScene(std::string const& virtualPath);
	// Request scene creation and defer it when the current scene has unsaved changes.
	// e.g. RequestCreateScene("res://Scenes/Main.tscn", "Main")
	void RequestCreateScene(std::string const& virtualPath, std::string const& rootName);
	bool LoadScene(std::string const& virtualPath);
	// Save the current scene path and clear its dirty state on success.
	// e.g. SaveScene()
	bool SaveScene();
	// Mark the current scene as having unsaved editor changes.
	// e.g. MarkSceneDirty()
	void MarkSceneDirty();
	bool IsSceneDirty() const;
	bool HasScene() const;
	std::string GetCurrentSceneName() const;

	// --- Mouse event routing ---
	void OnMouseMove(Vec2 screenPos, Vec2 delta);
	void OnMouseDown(int keyCode, Vec2 screenPos);
	void OnMouseUp(int keyCode, Vec2 screenPos);

private:
	void OnReady() override;
	void OnProcess(float deltaSeconds) override;
	bool CreateScene(std::string const& virtualPath, std::string const& rootName);
	void RenderUnsavedScenePopup();
	void ExecutePendingSceneAction();

public:
	EditorSelection m_selection;
	EditorGizmos*   m_editorGizmos = nullptr;
	EditorCamera*   m_editorCamera = nullptr;
	EditorUI*       m_editorUI     = nullptr;

private:
	static EditorNode* s_instance;

	EditorData      m_editorData;
	EditorUIContext m_uiContext;

	enum class PendingSceneAction
	{
		None,
		Load,
		Create
	};

	PendingSceneAction m_pendingSceneAction = PendingSceneAction::None;
	std::string        m_pendingScenePath;
	std::string        m_pendingSceneRootName;
	bool               m_openUnsavedScenePopup = false;
};
