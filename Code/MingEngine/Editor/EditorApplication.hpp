#pragma once

class Clock;
class EditorController;
class IProjectModule;
class SceneTree;

void RegisterEditorTypes();

enum class AppStateType
{
	Editor,
	Runtime,
};

class AppStateBase
{
public:
	virtual ~AppStateBase() = default;

	virtual void OnEnter() {}
	virtual void OnExit() {}
	virtual void BeginFrame() {}
	virtual void Update(float deltaSeconds) = 0;
	virtual void EndFrame() {}
};

class AppEditorState : public AppStateBase
{
public:
	explicit AppEditorState(IProjectModule& project);
	~AppEditorState() override;

	void OnEnter() override;
	void OnExit() override;
	void Update(float deltaSeconds) override;
	void BeginFrame() override;
	void EndFrame() override;

private:
	void BuildEditorScene();
	void HandleDebugInput();
	void UpdateDebugOverlay(float systemDeltaSeconds);

private:
	Clock*            m_editorClock      = nullptr;
	SceneTree*        m_sceneTree        = nullptr;
	EditorController* m_editorController = nullptr;
	IProjectModule&   m_project;
	bool              m_isSlowMode       = false;
};

class AppRuntimeState : public AppStateBase
{
public:
	AppRuntimeState();
	~AppRuntimeState() override;

	void OnEnter() override;
	void OnExit() override;
	void Update(float deltaSeconds) override;
	void BeginFrame() override;
	void EndFrame() override;

private:
	void BuildRuntimeScene();

private:
	Clock*     m_runtimeClock = nullptr;
	SceneTree* m_sceneTree    = nullptr;
};

class AppStateMachine
{
public:
	AppStateMachine();
	~AppStateMachine();

	void Startup(AppStateType initialState, IProjectModule& project);
	void Shutdown();

	void RequestTransition(AppStateType stateID);
	void ToggleEditorRuntime();

	void         BeginFrame();
	void         Update(float deltaSeconds);
	void         EndFrame();
	AppStateType GetCurrentStateType() const { return m_currentStateType; }

private:
	AppStateBase* GetState(AppStateType stateID) const;
	void          ApplyPendingTransition();

private:
	AppEditorState*  m_editorState      = nullptr;
	AppRuntimeState* m_runtimeState     = nullptr;
	AppStateType     m_currentStateType = AppStateType::Editor;
	AppStateType     m_pendingStateType = AppStateType::Editor;
	bool             m_hasCurrentState  = false;
	bool             m_hasPendingState  = false;
};