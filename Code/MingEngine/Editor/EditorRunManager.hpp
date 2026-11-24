#pragma once

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/EditorData.hpp"

class EditorRunManager
{
	friend class EditorNode;
	friend class EditorSceneManager;
	friend class EditorCSharpBuildManager;

private:
	enum class State
	{
		Idle,
		Saving,
		Preparing,
		Building,
		Launching
	};

public:
	explicit EditorRunManager(EditorNode& editor);

	void RequestPlay();
	void Stop();
	void Update();
	void Cancel();
	void BuildFinished(bool success);
	bool IsPreparingPlay() const;
	void LaunchPreparedGame(bool scripted);

private:
	EditorNode& m_editor;
	int m_playDelayFrames = 0;
	State m_state = State::Idle;
};
