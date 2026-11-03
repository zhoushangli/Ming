#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Editor/EditorData.hpp"
#include "MingEngine/Editor/EditorUndoRedo.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Resource/Theme.hpp"
#include <functional>
#include <mutex>
#include <vector>

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

public:
	enum class SceneActionChoice
	{
		Save,
		Discard,
		Cancel
	};

private:
	enum class SaveResult
	{
		Saved,
		AwaitingPath,
		Failed
	};

	enum class PendingSceneAction
	{
		None,
		New,
		Close,
		Play,
		Load,
		Create,
		Quit
	};

public:
	explicit EditorNode(float editorScale = 1.25f);
	~EditorNode() override;

	Ref<Theme> GetEditorTheme() const { return m_editorTheme; }
	float      GetEditorScale() const { return m_editorScale; }
	bool       SetEditorScale(float scale);

	static EditorNode* Get();
	bool               HandleGuiInput(InputEvent const& event);
	bool               IsEditingNativeControl() const;
	void               ResolvePendingSceneAction(SceneActionChoice choice);
	bool               IsSceneNavigationActive() const;
	void               RequestQuit();

	EditorUndoRedo&       GetUndoRedo() { return m_undoRedo; }
	EditorUndoRedo const& GetUndoRedo() const { return m_undoRedo; }

	EditorSelection&       GetSelection();
	EditorSelection const& GetSelection() const;

	EditorCamera* GetEditorCamera() const { return m_editorCamera; }

	void RequestLoadScene(VirtualPath const& virtualPath);
	void RequestOpenScene();
	void RequestCreateScene(
		VirtualPath const& virtualPath, std::string const& rootName, std::function<void()> completed = {});
	bool        LoadScene(VirtualPath const& virtualPath);
	bool        SaveScene();
	bool        SaveSceneAs(VirtualPath const& path);
	void        RequestSaveSceneAs();
	void        RequestNewScene();
	void        RequestCloseScene(std::function<void()> completed = {});
	void        RequestPlay();
	bool        GenerateCSharpProject();
	bool        BuildCSharpProject();
	bool        ReloadCSharpAssembly();
	void        NewRootCreated();
	bool        ContainsCurrentScene(VirtualPath const& path) const;
	void        FilesMoved(VirtualPath const& before, VirtualPath const& after);
	bool        FilesRemoved(VirtualPath const& path);
	bool        SetMainScene(VirtualPath const& path);
	VirtualPath GetCurrentScenePath() const { return m_editorData.m_currentScenePath; }

	void        MarkSceneDirty();
	bool        IsSceneDirty() const;
	bool        HasScene() const;
	std::string GetCurrentSceneName() const;

	void OnMouseMove(Vector2 screenPos, Vector2 delta);
	void OnMouseDown(int keyCode, Vector2 screenPos);
	void OnMouseUp(int keyCode, Vector2 screenPos);

protected:
	static void BindMethods();

private:
	void       OnReady() override;
	void       OnProcess(float deltaSeconds) override;
	bool       CreateScene(VirtualPath const& virtualPath, std::string const& rootName);
	void       RenderUnsavedScenePopup();
	void       ExecutePendingSceneAction();
	void       RequestSceneAction(PendingSceneAction action, std::function<void()> completed = {});
	void       CancelPendingSceneAction();
	SaveResult SaveCurrentScene();
	void       ClearSceneState();

public:
	EditorSelection m_selection;
	EditorGizmos*   m_editorGizmos = nullptr;
	EditorCamera*   m_editorCamera = nullptr;
	EditorUI*       m_editorUI     = nullptr;

private:
	static EditorNode* s_instance;

	Ref<Theme>      m_baseEditorTheme;
	Ref<Theme>      m_editorTheme;
	float           m_editorScale = 1.25f;
	EditorData      m_editorData;
	EditorUndoRedo  m_undoRedo;
	EditorUIContext m_uiContext;

	PendingSceneAction                               m_pendingSceneAction = PendingSceneAction::None;
	SaveResult                                       m_lastSaveResult     = SaveResult::Failed;
	VirtualPath                                      m_pendingScenePath;
	std::string                                      m_pendingSceneRootName;
	bool                                             m_openUnsavedScenePopup = false;
	bool                                             m_waitingForSave        = false;
	bool                                             m_resumePending         = false;
	std::function<void()>                            m_afterClose;
	std::mutex                                       m_messageMutex;
	std::vector<std::pair<std::string, std::string>> m_messages;
};
