#pragma once

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/EditorData.hpp"

class EditorCSharpBuildManager
{
	friend class EditorNode;
	friend class EditorSceneManager;
	friend class EditorRunManager;

public:
	explicit EditorCSharpBuildManager(EditorNode& editor);
	~EditorCSharpBuildManager();

	bool GenerateCSharpProject();
	bool OpenCSharpScript(VirtualPath const& path);

	bool BuildCSharpProject();
	bool BeginCSharpBuild();
	void UpdateCSharpBuild();

	bool ReloadCSharpAssembly();

private:
	EditorNode&                           m_editor;
	bool                                  m_buildInProgress = false;
	int32_t                               m_buildExitCode = -1;
	std::chrono::steady_clock::time_point m_buildStarted;
};
