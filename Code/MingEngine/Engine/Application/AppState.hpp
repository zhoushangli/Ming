#pragma once

class App;

class AppState
{
public:
	virtual ~AppState() = default;

	// Return false when mode initialization fails; App will still call Exit before destroying the tree.
	// e.g. GameState rejects a missing main scene and cleans up through the normal exit path.
	virtual bool Enter(App& app) = 0;
	virtual void Update(App& app, float deltaSeconds) = 0;
	// Release mode resources while engine services and the scene tree are available.
	// e.g. EditorState stops its child process; App then destroys nodes before deleting the state.
	virtual void Exit(App& app) = 0;
	virtual bool CanQuit(App& app);
};
