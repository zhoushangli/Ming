#pragma once

#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <string>
#include <string_view>

class Renderer;

class RenderServer
{
public:
	explicit RenderServer(RendererServerConfig config);
	~RenderServer();

	RenderServer(RenderServer const&)            = delete;
	RenderServer& operator=(RenderServer const&) = delete;

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	void Render();

#pragma region Instance API

	RID  InstanceCreate();
	void InstanceFree(RID instance);

	void InstanceSetBase(RID instance, RID base);
	void InstanceSetTransform(RID instance, Matrix4x4 const& transform);
	void InstanceSetTint(RID instance, Color tint);
	void InstanceSetVisible(RID instance, bool visible);
	void InstanceSetScenario(RID instance, RID scenario);

#pragma endregion

#pragma region Viewport API

	// Viewport lifecycle:
	// 1) A Viewport creates its RID and initial size before entering a SceneTree.
	// 2) Entering / leaving a SceneTree only toggles the active flag.
	// 3) The RID and its GPU resources are released on destruction.
	RID  ViewportCreate();
	void ViewportFree(RID viewport);
	void ViewportSetActive(RID viewport, bool active);
	void ViewportSetResolution(RID viewport, IntVec2 size);
	void ViewportBeginFrame(RID viewport);

	// Camera binding:
	// 1) Camera3D binds itself on EnterTree and clears on ExitTree.
	// 2) A Viewport without a camera still renders UI, but world passes are skipped.
	void ViewportSetCamera(RID viewport, RID camera);
	void ViewportFreeCamera(RID viewport, RID camera);

	void        ViewportSubmitRenderRequest(RID viewport, RenderRequest const& request);
	GPUTexture* ViewportGetTexture(RID viewport) const;
	void        ViewportSetScenario(RID viewport, RID scenario);

#pragma endregion

#pragma region Mesh API

	RID  MeshCreate(Ref<MeshResource> const& mesh);
	void MeshFree(RID mesh);

#pragma endregion

#pragma region Scenario API

	RID  ScenarioCreate();
	void ScenarioFree(RID scenario);

#pragma endregion

#pragma region Light API

	RID  LightCreate(LightType type);
	void LightFree(RID rid);
	void LightSetColor(RID rid, Color const& color);
	void LightSetIntensity(RID rid, float intensity);
	void LightSetRange(RID rid, float range);
	void LightSetAttenuation(RID rid, float attenuation);
	void LightSetSpotAngle(RID rid, float angle);
	void LightSetSpotAttenuation(RID rid, float attenuation);

#pragma endregion

#pragma region Camera API

	RID  CameraCreate();
	void CameraFree(RID camera);

	void CameraSetTransform(RID camera, Matrix4x4 const& cameraToWorld);
	void CameraSetPerspective(RID camera, float fovDegrees, float nearZ, float farZ);
	void CameraSetOrthographic(RID camera, float size, float nearZ, float farZ);

#pragma endregion

	Shader* CreateShader(std::string const& name, std::string const& source, std::string const& sourcePath = {});

	Ref<ShaderResource> GetBuiltinShaderResource(std::string const& name, std::string_view source);

	GPUTexture* CreateGPUTexture(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t const* data);

	void DestroyTexture(GPUTexture* texture);

	VertexBuffer* CreateVertexBuffer(unsigned int size, unsigned int stride);
	VertexBuffer* CreateVertexBuffer(void const* data, unsigned int size, unsigned int stride);

	IndexBuffer* CreateIndexBuffer(unsigned int size);
	IndexBuffer* CreateIndexBuffer(void const* data, unsigned int size, unsigned int stride);

	void UpdateVertexBuffer(VertexBuffer* buffer, void const* data, unsigned int size);

	void CopyCPUToGPU(void const* data, unsigned int size, VertexBuffer* buffer);

	bool        InitImGui();
	void        ShutdownImGui();
	void        BeginImGuiFrame();
	void        RenderImGui(ImDrawData* drawData);
	ImTextureID GetImGuiTextureID(GPUTexture* texture) const;

private:
	static bool OnWindowResized(EventArgs& args);

	RenderRequest BuildInstanceRenderRequest(Instance const& instance);

private:
	Renderer* m_renderer = nullptr;
	bool      m_started  = false;

	RIDOwner<LightData>    m_lightOwner;
	RIDOwner<MeshData>     m_meshOwner;
	RIDOwner<ScenarioData> m_scenarioOwner;
	RIDOwner<Instance>     m_instanceOwner;
	RIDOwner<ViewportData> m_viewportOwner;
	RIDOwner<CameraData>   m_cameraOwner;
};
