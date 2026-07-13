#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Render/D3D11RenderBackend.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"

#include <string>

class ViewportInfo;
struct RenderRequest;

class Renderer : public SystemBase
{
	MCLASS(Renderer, SystemBase)

public:
	Renderer(RendererConfig config);
	~Renderer();

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;
	void CreateRenderingContext();

	// Per-Viewport resource lifetime:
	// 1) Create lazily before the first render.
	// 2) Resize only this Viewport's targets.
	// 3) Destroy before the owning Viewport or Renderer shuts down.
	void RenderViewport(ViewportInfo& viewport);
	void ResizeViewport(ViewportInfo& viewport, IntVec2 dimensions);
	void DestroyViewportResources(ViewportInfo& viewport);
	void CopyTextureToBackBuffer(GPUTexture* colorTexture);

	Shader* CreateShader(std::string const& shaderVirtualPath, std::string const& shaderSource);

	GPUTexture* CreateGPUTexture(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t const* texelData);
	GPUTexture* CreateRenderTargetTexture(char const* name, IntVec2 dimensions);
	GPUTexture* CreateDepthStencilTexture(char const* name, IntVec2 dimensions);
	void        DestroyTexture(GPUTexture* texture);

	VertexBuffer*   CreateVertexBuffer(const unsigned int size, unsigned int stride);
	VertexBuffer*   CreateVertexBuffer(void const* data, const unsigned int byteSize, unsigned int stride);
	ConstantBuffer* CreateConstantBuffer(const unsigned int size);
	IndexBuffer*    CreateIndexBuffer(const unsigned int size);
	IndexBuffer*    CreateIndexBuffer(void const* data, const unsigned int byteSize, const unsigned int stride);

	void UpdateVertexBuffer(VertexBuffer* vertexBuffer, void const* data, unsigned int byteSize);

	void CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vertexBuffer);
	void CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* constantBuffer);
	void CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* indexBuffer);
	void BindConstantBuffer(ConstantBuffer* constantBuffer, int slot);

	// This function is specifically for initializing the ImGui D3D11 backend in ImGuiSystem
	void InitImGuiD3D11Backend();
	void BindBackBuffer();
	void ResizeBackBuffer(IntVec2 newDimensions);

	void SetViewport(IntVec2 dimensions, IntVec2 topLeft = IntVec2::Zero);
	void ClearSceneTargets(ViewportInfo const& viewport);

	static void BindMethods();

private:
	void ExecuteRenderRequest(RenderRequest const& request);

	void PrepareConstants(ViewportInfo const& viewport);
	void RenderOpaque(ViewportInfo& viewport);
	void RenderSkybox(ViewportInfo const& viewport);
	void RenderPostProcess(ViewportInfo& viewport);
	void RenderUI(ViewportInfo const& viewport);

private:
	RendererConfig      m_config;
	D3D11RenderBackend* m_renderBackend = nullptr;

	Ref<ShaderResource> m_defaultShaderResource;
	Ref<ShaderResource> m_postProcessCopyShaderResource;
};
