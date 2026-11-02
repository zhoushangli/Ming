#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Editor/EditorData.hpp"
#include "MingEngine/Editor/EditorUndoRedo.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Resource/Theme.hpp"

class Camera3D;
class EditorCamera;
class EditorUI;
class EditorGizmos;

class EditorSelection
{
public:
	ObjectID GetSelected() const;
	void     SetSelected(ObjectID nodeID);
	void     Clear();

private:
	ObjectID m_selectedNodeID = ObjectID::Invalid;
};

class EditorNode : public Node
{
	MCLASS(EditorNode, Node);

private:
	enum class PendingSceneAction
	{
		None,
		Load,
		Create,
		Quit
	};


public:
	explicit EditorNode(float editorScale = 1.25f);
	~EditorNode() override;

	Ref<Theme> GetEditorTheme() const { return m_editorTheme; }

	static EditorNode* Get();
	bool HandleGuiInput(InputEvent const& event);
	bool IsEditingNativeControl() const;
	void ResolvePendingSceneAction(int choice);
	bool IsSceneNavigationActive() const;
	void RequestQuit();

	EditorUndoRedo&       GetUndoRedo() { return m_undoRedo; }
	EditorUndoRedo const& GetUndoRedo() const { return m_undoRedo; }

	EditorSelection&       GetSelection();
	EditorSelection const& GetSelection() const;

	EditorCamera* GetEditorCamera() const { return m_editorCamera; }

	void RequestLoadScene(VirtualPath const& virtualPath);
	void RequestCreateScene(VirtualPath const& virtualPath, std::string const& rootName);
	bool LoadScene(VirtualPath const& virtualPath);
	bool SaveScene();

	void        MarkSceneDirty();
	bool        IsSceneDirty() const;
	bool        HasScene() const;
	std::string GetCurrentSceneName() const;

	void OnMouseMove(Vector2 screenPos, Vector2 delta);
	void OnMouseDown(int keyCode, Vector2 screenPos);
	void OnMouseUp(int keyCode, Vector2 screenPos);

private:
	void OnReady() override;
	void OnProcess(float deltaSeconds) override;
	bool CreateScene(VirtualPath const& virtualPath, std::string const& rootName);
	void RenderUnsavedScenePopup();
	void ExecutePendingSceneAction();

public:
	EditorSelection m_selection;
	EditorGizmos*   m_editorGizmos = nullptr;
	EditorCamera*   m_editorCamera = nullptr;
	EditorUI*       m_editorUI     = nullptr;

private:
	static EditorNode* s_instance;

	Ref<Theme> m_baseEditorTheme;
	Ref<Theme> m_editorTheme;
	float m_editorScale = 1.25f;
	EditorData      m_editorData;
	EditorUndoRedo  m_undoRedo;
	EditorUIContext m_uiContext;


	PendingSceneAction m_pendingSceneAction = PendingSceneAction::None;
	VirtualPath        m_pendingScenePath;
	std::string        m_pendingSceneRootName;
	bool               m_openUnsavedScenePopup = false;
};
