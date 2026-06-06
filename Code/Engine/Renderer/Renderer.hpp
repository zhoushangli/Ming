#pragma once

#include "Engine/Renderer/D3D11RenderBackend.hpp"

class ViewportInfo;
struct RenderRequest;

class Renderer
{
public:
	Renderer(RendererConfig config);
	~Renderer();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();
	void CreateRenderingContext();

	// Per-Viewport resource lifetime:
	// 1) Create lazily before the first render.
	// 2) Resize only this Viewport's targets.
	// 3) Destroy before the owning Viewport or Renderer shuts down.
	void CreateViewportResources(ViewportInfo& viewport);
	void ResizeViewport(ViewportInfo& viewport, IntVec2 dimensions);
	void DestroyViewportResources(ViewportInfo& viewport);
	void RenderViewport(ViewportInfo& viewport);
	void CopyViewportToBackBuffer(ViewportInfo const& viewport);

	Shader* CreateOrGetShader(char const* shaderName);

	Texture*    CreateOrGetTexture(char const* fileDataPath);
	Texture*    CreateTextureFromImage(const Image& image);
	Texture*    CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData);
	Texture*    CreateRenderTargetTexture(char const* name, IntVec2 dimensions);
	Texture*    CreateDepthStencilTexture(char const* name, IntVec2 dimensions);
	BitmapFont* CreateOrGetBitmapFont(char const* fontFilePathNameWithNoExtension);

	VertexBuffer*   CreateVertexBuffer(const unsigned int size, unsigned int stride);
	VertexBuffer*   CreateVertexBuffer(std::vector<Vertex> const& verts);
	ConstantBuffer* CreateConstantBuffer(const unsigned int size);
	IndexBuffer*    CreateIndexBuffer(const unsigned int size);
	IndexBuffer*    CreateIndexBuffer(std::vector<unsigned int> const& indexes);

	void CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vertexBuffer);
	void CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* constantBuffer);
	void CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* indexBuffer);

	Texture* GetTextureFromFileName(char const* fileName);

	// This function is specifically for initializing the ImGui D3D11 backend in ImGuiSystem
	void     InitImGuiD3D11Backend();
	void     BindBackBuffer();

private:
	void ExecuteRenderRequest(RenderRequest const& request);
	void EnsureViewport(ViewportInfo& viewport);
	void ClearSceneTargets(ViewportInfo const& viewport);
	void CopyTextureToBackBuffer(Texture* colorTexture);

	void PrepareConstants(ViewportInfo const& viewport);
	void RenderOpaque(ViewportInfo const& viewport);
	void RenderSkybox(ViewportInfo const& viewport);
	void RenderPostProcess(ViewportInfo& viewport);
	void RenderUI(ViewportInfo const& viewport);

private:
	RendererConfig      m_config;
	D3D11RenderBackend* m_renderBackend = nullptr;
	Shader* m_postProcessCopyShader = nullptr;
};
