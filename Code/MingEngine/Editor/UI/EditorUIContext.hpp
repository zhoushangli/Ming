#pragma once

class EditorSelection;
class EditorUI;
class FileSystem;
class SceneTree;

struct EditorUIContext
{
	// These dependencies are borrowed for one immediate-mode Render call.
	SceneTree* m_sceneTree       = nullptr;
	EditorSelection* m_selection = nullptr;
	EditorUI* m_editorUI         = nullptr;
	FileSystem* m_fileSystem     = nullptr;
	
	bool m_isViewportImageHovered = false;
};
