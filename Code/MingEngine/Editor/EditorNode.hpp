#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Editor/EditorSelection.hpp"
#include "MingEngine/Editor/EditorUndoRedo.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Resource/Theme.hpp"
#include <functional>
#include <chrono>
#include <mutex>
#include <vector>

class Camera3D;
class EditorCamera;
class EditorUI;
class EditorGizmos;
class EditorRunManager;
class EditorCSharpBuildManager;
class EditorSceneManager;
class EditorChecker;

class EditorNode : public Node
{
	MCLASS(EditorNode, Node);
	friend class EditorRunManager;
	friend class EditorCSharpBuildManager;
	friend class EditorSceneManager;
	friend class EditorChecker;

public:
	enum class SceneActionChoice
	{
		Save,
		Discard,
		Cancel
	};

public:
	explicit EditorNode(float editorScale = 1.25f, std::function<void()> pollGameProcess = {});
	~EditorNode() override;

	static EditorNode* Get();

	Ref<Theme> GetEditorTheme() const { return m_editorTheme; }
	float      GetEditorScale() const { return m_editorScale; }
	bool       SetEditorScale(float scale);

	EditorUndoRedo&       GetUndoRedo() { return m_undoRedo; }
	EditorUndoRedo const& GetUndoRedo() const { return m_undoRedo; }
	EditorSelection&       GetSelection();
	EditorSelection const& GetSelection() const;
	EditorCamera*  GetEditorCamera() const { return m_editorCamera; }
	EditorChecker* GetChecker() const { return m_checker; }

	bool HandleGuiInput(InputEvent const& event);
	bool IsEditingNativeControl() const;
	bool IsSceneNavigationActive() const;

	void CheckProject(bool full = false);
	bool IsCheckingProject() const;
	void ImportExternalFiles(std::vector<std::string> const& paths, Vector2 position);
	void NotifyFileSaved(VirtualPath const& path);
	void NotifySceneLoaded(VirtualPath const& path);
	void ShutdownBackgroundWork();

	void MarkSceneDirty();
	void MarkResourceDirty(Object* resource);
	bool IsSceneDirty() const;
	bool HasScene() const;
	bool SaveAll();
	std::string GetCurrentSceneName() const;
	VirtualPath GetCurrentScenePath() const;

	void RequestLoadScene(VirtualPath const& virtualPath);
	void RequestOpenScene();
	void RequestCreateScene(
		VirtualPath const& virtualPath, std::string const& rootName, std::function<void()> completed = {});
	void RequestNewScene();
	void RequestCloseScene(std::function<void()> completed = {});
	void RequestSaveSceneAs();
	void ResolvePendingSceneAction(SceneActionChoice choice);
	void RequestQuit();

	bool LoadScene(VirtualPath const& virtualPath);
	bool SaveScene();
	bool SaveSceneAs(VirtualPath const& path);
	bool SetMainScene(VirtualPath const& path);
	void NewRootCreated();

	bool ContainsCurrentScene(VirtualPath const& path) const;
	void FilesMoved(VirtualPath const& before, VirtualPath const& after);
	bool FilesRemoved(VirtualPath const& path);

	void RequestPlay();
	void RequestStop();
	bool IsPreparingPlay() const;

	bool    IsBuildingCSharpProject() const;
	int32_t GetCSharpBuildExitCode() const;
	bool    GenerateCSharpProject();
	bool    OpenCSharpScript(VirtualPath const& path);

	// Start a menu build and return whether the build process was started.
	// e.g. BuildCSharpProject() returns before the compiler finishes.
	bool BuildCSharpProject();
	bool ReloadCSharpAssembly();

	void OnMouseMove(Vector2 screenPos, Vector2 delta);
	void OnMouseDown(int keyCode, Vector2 screenPos);
	void OnMouseUp(int keyCode, Vector2 screenPos);

protected:
	static void BindMethods();

private:
	void _Ready() override;
	void OnNotification(int notification);

public:
	EditorGizmos* m_editorGizmos = nullptr;
	EditorCamera* m_editorCamera = nullptr;
	EditorUI*     m_editorUI     = nullptr;

private:
	static EditorNode* s_instance;

	EditorSelection*          m_selection      = nullptr;
	EditorChecker*            m_checker = nullptr;
	EditorSceneManager*       m_sceneManager   = nullptr;
	EditorCSharpBuildManager* m_buildManager   = nullptr;
	EditorRunManager*         m_runManager     = nullptr;

	Ref<Theme> m_baseEditorTheme;
	Ref<Theme> m_editorTheme;
	float      m_editorScale = 1.25f;

	EditorUndoRedo  m_undoRedo;
	EditorUIContext m_uiContext;

	std::function<void()>                            m_pollGameProcess;
	std::mutex                                       m_messageMutex;
	std::vector<std::pair<std::string, std::string>> m_messages;
};
