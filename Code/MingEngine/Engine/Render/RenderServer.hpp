#pragma once

#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"
#include "MingEngine/Scene/Resource/MaterialResource.hpp"
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

#pragma region Canvas API

	RID  CanvasItemCreate();
	void CanvasItemFree(RID item);
	void CanvasItemSetViewport(RID item, RID viewport);
	void CanvasItemSetParent(RID item, RID parent);
	void CanvasItemSetPosition(RID item, Vector2 const& position);
	void CanvasItemSetVisible(RID item, bool visible);
	void CanvasItemClear(RID item);

	void CanvasItemAddRect(RID item, AABB2 const& rect, Color const& color);
	void CanvasItemAddTextureRect(
		RID item, Ref<TextureResource> const& texture, Rect2 const& rect, bool isTiling = false);
	void CanvasItemAddTextureRectRegion(
		RID item, Ref<TextureResource> const& texture, Rect2 const& rect, Rect2 const& sourceRect, Color const& color);

#pragma endregion

#pragma region CanvasLayer API

	RID  CanvasLayerCreate();
	void CanvasLayerFree(RID layer);
	void CanvasLayerSetViewport(RID layer, RID viewport);
	void CanvasLayerAddChild(RID layer, RID child);
	void CanvasLayerRemoveChild(RID layer, RID child);

#pragma endregion

#pragma region Instance API

	RID  InstanceCreate();
	void InstanceFree(RID instance);

	void InstanceSetBase(RID instance, RID base);
	void InstanceSetTransform(RID instance, Matrix4x4 const& transform);
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
	void        ViewportSetPresentToScreen(RID viewport, bool presentToScreen);

#pragma endregion

#pragma region Mesh API

	// Mesh registration:
	// 1) MeshCreate() uploads the CPU data and owns the GPU buffers until MeshFree().
	// 2) MeshRefresh() rebuilds the GPU buffers of an existing RID, so instances keep rendering.
	// 3) One MeshResource registers once, no matter how many instances share it.
	RID  MeshCreate(MeshResource const& meshResource);
	void MeshFree(RID mesh);
	void MeshRefresh(RID mesh, MeshResource const& meshResource);

#pragma endregion

#pragma region Texture API

	// Texture registration:
	// 1) TextureCreate() uploads the CPU image and owns the GPUTexture until TextureFree().
	// 2) TextureRefresh() rebuilds the GPUTexture of an existing RID, so meshes keep sampling it.
	// 3) One TextureResource registers once, no matter how many meshes reference it.
	RID  TextureCreate(TextureResource const& textureResource);
	void TextureFree(RID texture);
	void TextureRefresh(RID texture, TextureResource const& textureResource);

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

	// Default resources every system can fall back to; created by Startup() and released before the Renderer.
	// e.g. meshes without a material draw with the default material, requests without a shader use the default unlit
	Ref<ShaderResource>   GetDefaultUnlitShaderResource() const;
	Ref<ShaderResource>   GetDefaultLitShaderResource() const;
	Ref<MaterialResource> GetDefaultMaterialResource() const;

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

	void          PrepareViewportData(ViewportData* viewportData);
	RenderRequest BuildInstanceRenderRequest(Instance const* instance);

	// Fill shader, textures, tint and draw states of one request from a material.
	// e.g. BuildInstanceRenderRequest() calls it with the mesh material or the default material
	void ApplyMaterialToRequest(MaterialResource const& materialResource, RenderRequest& request);

	// GPU buffer registrations stay private because only MeshCreate()/MeshFree() use them.
	RID  RegisterVertexBuffer(VertexBuffer* vertexBuffer);
	void FreeVertexBuffer(RID vertexBuffer);
	RID  RegisterIndexBuffer(IndexBuffer* indexBuffer);
	void FreeIndexBuffer(RID indexBuffer);

	// Rebuild the GPU resources a registration owns from the CPU data of its resource.
	// e.g. MeshCreate() and MeshRefresh() both go through PrepareMeshData()
	bool PrepareMeshData(MeshData& data, MeshResource const& meshResource);
	bool PrepareTextureData(TextureData& data, TextureResource const& textureResource);

	void PrepareCanvasLayerRequests(RID layer, RID viewport, ViewportData* viewportData);
	void PrepareCanvasItemRequests(RID item, RID viewport, ViewportData* viewportData, Vector2 const& parentPosition);

private:
	Renderer* m_renderer = nullptr;
	bool      m_started  = false;

	Ref<ShaderResource>   m_defaultUnlit;
	Ref<ShaderResource>   m_defaultUI;
	Ref<ShaderResource>   m_defaultLit;
	Ref<MaterialResource> m_defaultMaterial;

	RIDOwner<CanvasItemData>   m_canvasItemOwner;
	RIDOwner<CanvasLayerData>  m_canvasLayerOwner;
	RIDOwner<LightData>        m_lightOwner;
	RIDOwner<MeshData>         m_meshOwner;
	RIDOwner<VertexBufferData> m_vertexBufferOwner;
	RIDOwner<IndexBufferData>  m_indexBufferOwner;
	RIDOwner<TextureData>      m_textureOwner;
	RIDOwner<ScenarioData>     m_scenarioOwner;
	RIDOwner<Instance>         m_instanceOwner;
	RIDOwner<ViewportData>     m_viewportOwner;
	RIDOwner<CameraData>       m_cameraOwner;
};
