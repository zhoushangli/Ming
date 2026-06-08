#pragma once

class EditorSelection;
class SceneTree;

struct EditorUIContext
{
	// These dependencies are borrowed for one immediate-mode Render call.
	SceneTree*       m_sceneTree = nullptr;
	EditorSelection* m_selection = nullptr;
};
