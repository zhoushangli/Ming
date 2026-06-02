#pragma once

#include "Engine/Renderer/D3D11RenderBackend.hpp"
#include "Engine/Renderer/Viewport.hpp"

#include <unordered_map>

class PostProcessChain;

enum class RenderRequestPass
{
	Opaque,
	Skybox,
	Transparent,
	UI,
};

struct RenderRequestInfo
{
	RenderRequestPass m_pass = RenderRequestPass::Opaque;

	Matrix4x4 m_modelToWorld = Matrix4x4::Identity;
	Rgba8     m_tint         = Rgba8::White;

	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer*  m_indexBuffer  = nullptr;

	Texture* m_diffuseTexture = nullptr;
	Shader*  m_shader         = nullptr;

	BlendMode      m_blendMode      = BlendMode::OPAQUE;
	DepthMode      m_depthMode      = DepthMode::READ_WRITE_LESS_EQUAL;
	RasterizerMode m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	SamplerMode    m_samplerMode    = SamplerMode::POINT_CLAMP;
};

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

	// Render Viewport is the main entry point for rendering a frame
	// It will execute all render requests that have been submitted, and then clear the list of render requests.
	void RenderViewport(Viewport const& viewport);
	void SubmitRenderRequest(RenderRequestInfo const& request);
	void ClearRenderRequests();

	// Temporary backend API forwarding. These keep existing callers compiling until render requests replace direct draw
	// calls.
	void BeginCamera(Camera const& camera);
	void EndCamera();
	void ClearScreen(Rgba8 const& clearColor);
	void SetBlendMode(BlendMode blendMode);
	void SetRasterizerMode(RasterizerMode rasterizerMode);
	void SetDepthMode(DepthMode depthMode);
	void SetStatesIfChanged();

	void DrawVertexArray(int numVertexes, Vertex const* vertexes);
	void DrawVertexArray(std::vector<Vertex> const& verts);
	void DrawVertexArray(std::vector<Vertex> const& verts, std::vector<unsigned int> const& vertIndexs);
	void DrawVertexBuffer(VertexBuffer* vertexBuffer);
	void DrawIndexedVertexBuffer(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer);

	void BindTexture(Texture* textureOrNull);
	void BindTexture(Texture* textureOrNull, unsigned int slot);
	void BindSampler(SamplerMode samplerMode, unsigned int slot = 0);
	void BindShader(Shader* shader);
	void BindModelConstants(Matrix4x4 const& modelToWorldTransform, Rgba8 const& modelColor);
	void BindLightConstants(LightConstants const& lightConstants);
	void
	BindLightConstants(Vec3 const& sunDirection, float sunIntensity, Rgba8 const& ambientColor, float ambientIntensity);
	void BindPostProcessConstants(Vec2 const& screenDimensions, float cameraNear, float cameraFar);
	void BindFrameConstants(float time, float deltaSeconds);

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

	void BeginEvent(std::string const& eventName);
	void EndEvent();

	ID3D11Device*        GetD3DDevice() const;
	ID3D11DeviceContext* GetD3DDeviceContext() const;
	void            SetViewport(IntVec2 dimensions, IntVec2 topLeft = IntVec2::Zero);
	void            ResizeBackBuffer(IntVec2 newDimensions);
	Texture*        GetTextureFromFileName(char const* fileName);

private:
	void RenderRequest(RenderRequestInfo const& request);
	void EnsureViewport(Viewport const& viewport);
	void ResizeSceneTargets(IntVec2 dimensions);
	void DestroySceneTargets();
	void ClearSceneTargets(Rgba8 const& clearColor);
	void BindSceneTargets(bool bindNormal);
	void CopyTextureToBackBuffer(Texture* colorTexture);
	void RenderOpaque(Viewport const& viewport);
	void RenderSkybox(Viewport const& viewport);
	void RenderPostProcess(Viewport const& viewport);
	void RenderUI(Viewport const& viewport);

private:
	RendererConfig      m_config;
	D3D11RenderBackend* m_renderBackend    = nullptr;
	PostProcessChain*   m_postProcessChain = nullptr;
	Shader*             m_postProcessCopyShader = nullptr;

	Texture* m_sceneColorTexture  = nullptr;
	Texture* m_sceneDepthTexture  = nullptr;
	Texture* m_sceneNormalTexture = nullptr;
	IntVec2  m_sceneTargetDimensions = IntVec2::Zero;

	std::unordered_map<RenderRequestPass, std::vector<RenderRequestInfo>> m_renderRequests;
};