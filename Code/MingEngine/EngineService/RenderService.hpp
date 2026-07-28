#pragma once

#include <vector>

#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"

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

	int  CreateLight(LightType type);
	void FreeLight(int rid);
	void SetLightTransform(int rid, Matrix4x4 const& transform);
	void SetLightColor(int rid, Color const& color);
	void SetLightIntensity(int rid, float intensity);
	void SetLightRange(int rid, float range);
	void SetLightAttenuation(int rid, float attenuation);
	void SetLightSpotAngle(int rid, float angle);
	void SetLightSpotAttenuation(int rid, float attenuation);

private:
	struct LightEntry
	{
		int       m_rid = -1;
		LightInfo m_info;
	};

	LightEntry* FindLight(int rid);

	static bool OnWindowResized(EventArgs& args);
	static bool OnEditorViewportResized(EventArgs& args);

private:
	std::vector<Viewport*>  m_viewports;
	std::vector<LightEntry> m_lights;
	int                     m_nextLightRid = 0;
};
