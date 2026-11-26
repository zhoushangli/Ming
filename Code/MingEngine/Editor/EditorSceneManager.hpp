#pragma once

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/EditorData.hpp"

class EditorSceneManager
{
	friend class EditorNode;
	friend class EditorCSharpBuildManager;
	friend class EditorRunManager;

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
		Continue,
		Load,
		Create,
		Quit
	};

	using SceneActionChoice = EditorNode::SceneActionChoice;

public:
	explicit EditorSceneManager(EditorNode& editor);

	bool        HasScene() const;
	bool        IsSceneDirty() const;
	std::string GetCurrentSceneName() const;
	bool        ContainsCurrentScene(VirtualPath const& path) const;

	void Update();
	void MarkSceneDirty();
	void NewRootCreated();

	void RequestNewScene();
	void RequestCloseScene(std::function<void()> completed = {});
	void RequestOpenScene();
	void RequestLoadScene(VirtualPath const& virtualPath);
	void RequestCreateScene(
		VirtualPath const& virtualPath, std::string const& rootName, std::function<void()> completed = {});

	bool LoadScene(VirtualPath const& virtualPath);
	bool SaveScene();
	bool SaveSceneAs(VirtualPath const& path);
	void RequestSaveSceneAs();
	bool SetMainScene(VirtualPath const& path);

	void ResolvePendingSceneAction(SceneActionChoice choice);
	void RequestQuit();

	void FilesMoved(VirtualPath const& before, VirtualPath const& after);
	bool FilesRemoved(VirtualPath const& path);

private:
	bool CreateScene(VirtualPath const& virtualPath, std::string const& rootName);
	SaveResult SaveCurrentScene();
	void ClearSceneState();
	void ClearSceneHistory();

	void RequestSceneAction(PendingSceneAction action, std::function<void()> completed = {});
	void CancelPendingSceneAction();
	void ExecutePendingSceneAction();
	void RenderUnsavedScenePopup();

private:
	EditorNode& m_editor;
	EditorData  m_editorData;

	PendingSceneAction m_pendingSceneAction = PendingSceneAction::None;
	SaveResult         m_lastSaveResult = SaveResult::Failed;
	VirtualPath        m_pendingScenePath;
	std::string        m_pendingSceneRootName;
	bool               m_openUnsavedScenePopup = false;
	bool               m_waitingForSave = false;
	bool               m_resumePending = false;
	bool               m_quitRequested = false;
	std::function<void()> m_afterClose;
};
