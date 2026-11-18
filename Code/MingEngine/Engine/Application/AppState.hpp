#pragma once

class App;

// AppState looks like a state machine for App state
// But actually it is more like for one time usage
// If the App wants to switch mode, it will just create a new process
// e.g. Editor Play launches a separate Game process with its own engine and physics world.
class AppState
{
public:
	virtual ~AppState() = default;

	// Return false when mode initialization fails; App will still call Exit before destroying the tree.
	// e.g. GameState rejects a missing main scene and cleans up through the normal exit path.
	virtual bool Enter(App& app) = 0;
	// Release mode resources while engine services and the scene tree are available.
	// e.g. EditorState stops its child process; App then destroys nodes before deleting the state.
	virtual void Exit(App& app) = 0;
	virtual bool CanQuit(App& app);
};
