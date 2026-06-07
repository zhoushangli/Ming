#pragma once

#include <vector>

#include "MingEngine/Engine/Event/EventSystem.hpp"

class Viewport;

class RenderService
{
public:
	RenderService()  = default;
	~RenderService() = default;

	void Startup();
	void Shutdown();

	// Viewport lifecycle:
	// 1) A Viewport registers after entering a SceneTree.
	// 2) It stays registered while the SceneTree owns it.
	// 3) It unregisters before leaving the SceneTree.
	void RegisterViewport(Viewport* viewport);
	void UnregisterViewport(Viewport* viewport);
	void Render() const;

private:
	static bool OnWindowResized(EventArgs& args);
	static bool OnEditorViewportResized(EventArgs& args);

private:
	std::vector<Viewport*> m_viewports;
};
