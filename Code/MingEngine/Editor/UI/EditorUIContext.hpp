#pragma once

class EditorSelection;
class EditorUI;
class FileSystem;
class SceneTree;
class Control;

struct EditorUIContext
{
	// These dependencies are borrowed by native panels and refreshed during editor updates.
	// e.g. InspectorPanel uses the shared selection without owning the selected scene node.
	Control*         m_rootControl = nullptr;
	SceneTree*       m_sceneTree   = nullptr;
	EditorSelection* m_selection   = nullptr;
	EditorUI*        m_editorUI    = nullptr;
	FileSystem*      m_fileSystem  = nullptr;

	bool m_isViewportImageHovered = false;
};
