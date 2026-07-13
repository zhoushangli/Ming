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

	void RequestLoadScene(std::string const& virtualPath);
	void RequestCreateScene(std::string const& virtualPath, std::string const& rootName);
	bool LoadScene(std::string const& virtualPath);
	bool SaveScene();

	void        MarkSceneDirty();
	bool        IsSceneDirty() const;
	bool        HasScene() const;
	std::string GetCurrentSceneName() const;

	// Launch the sibling Game executable as a child process.
	void PlayScene();
	// Stop the running game by sending WM_CLOSE to its window.
	void StopScene();
	bool IsPlaying() const;

	void OnMouseMove(Vec2 screenPos, Vec2 delta);
	void OnMouseDown(int keyCode, Vec2 screenPos);
	void OnMouseUp(int keyCode, Vec2 screenPos);

private:
	void OnReady() override;
	void OnProcess(float deltaSeconds) override;
	bool CreateScene(std::string const& virtualPath, std::string const& rootName);
	void RenderUnsavedScenePopup();
	void ExecutePendingSceneAction();
	// Check if the PIE process is still alive, and if not, clean up the state.
	// We do this to avoid the game process exit itself the leave a void handle in the editor
	void CheckPIEProcessAlive();

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

	// PIE state
	void*         m_pieProcessHandle = nullptr; // HANDLE
	unsigned long m_pieProcessId     = 0;       // DWORD
};
