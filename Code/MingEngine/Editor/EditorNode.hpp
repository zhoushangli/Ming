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

class EditorDragDrop
{
public:
	enum class Type
	{
		None,
		Node,
		FileSystemEntry
	};

	static constexpr char const* PayloadType = "EDITOR_DRAG";

	void BeginFrame() { m_dropAllowed = false; }
	void EndFrame(bool isDragging)
	{
		if (!isDragging)
		{
			Clear();
		}
	}

	void SetDragData(NodeHandle handle)
	{
		m_type        = Type::Node;
		m_virtualPath = {};
		m_nodeHandle  = handle;
	}
	void SetDragData(VirtualPath const& virtualPath)
	{
		m_type        = Type::FileSystemEntry;
		m_virtualPath = virtualPath;
		m_nodeHandle  = NodeHandle::Invalid;
	}

	bool TryGetData(NodeHandle& outHandle) const
	{
		if (m_type != Type::Node || m_nodeHandle == NodeHandle::Invalid)
		{
			return false;
		}

		outHandle = m_nodeHandle;
		return true;
	}
	bool TryGetData(VirtualPath& outVirtualPath) const
	{
		if (m_type != Type::FileSystemEntry || !m_virtualPath.IsValid())
		{
			return false;
		}

		outVirtualPath = m_virtualPath;
		return true;
	}

	void AllowDrop() { m_dropAllowed = true; }
	bool IsDropAllowed() const { return m_dropAllowed; }

	void Clear()
	{
		m_type        = Type::None;
		m_virtualPath = {};
		m_nodeHandle  = NodeHandle::Invalid;
	}

private:
	Type        m_type = Type::None;
	VirtualPath m_virtualPath;
	NodeHandle  m_nodeHandle  = NodeHandle::Invalid;
	bool        m_dropAllowed = false;
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

	void RequestLoadScene(VirtualPath const& virtualPath);
	void RequestCreateScene(VirtualPath const& virtualPath, std::string const& rootName);
	bool LoadScene(VirtualPath const& virtualPath);
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
	bool CreateScene(VirtualPath const& virtualPath, std::string const& rootName);
	void RenderUnsavedScenePopup();
	void ExecutePendingSceneAction();
	// Check if the PIE process is still alive, and if not, clean up the state.
	// We do this to avoid the game process exit itself the leave a void handle in the editor
	void CheckPIEProcessAlive();

public:
	EditorSelection m_selection;
	EditorDragDrop  m_dragDrop;
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
	VirtualPath        m_pendingScenePath;
	std::string        m_pendingSceneRootName;
	bool               m_openUnsavedScenePopup = false;

	// PIE state
	void*         m_pieProcessHandle = nullptr; // HANDLE
	unsigned long m_pieProcessId     = 0;       // DWORD
};
