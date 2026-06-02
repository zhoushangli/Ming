#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Renderer/PostProcessChain.hpp"
#include "Renderer.hpp"

Renderer::Renderer(RendererConfig config) : m_config(config) {}

Renderer::~Renderer() { Shutdown(); }

void Renderer::Startup()
{
	if (m_renderBackend != nullptr)
	{
		return;
	}

	m_renderBackend = new D3D11RenderBackend(m_config);
	m_renderBackend->Startup();
	m_postProcessCopyShader = m_renderBackend->CreateOrGetShader("Data/Shaders/PostProcessCopy");
	ResizeSceneTargets(g_engine->m_window->GetClientDimensions());
	m_postProcessChain = new PostProcessChain();
}

void Renderer::Shutdown()
{
	delete m_postProcessChain;
	m_postProcessChain = nullptr;

	if (m_renderBackend != nullptr)
	{
		DestroySceneTargets();
		m_renderBackend->Shutdown();
		delete m_renderBackend;
		m_renderBackend = nullptr;
	}

	m_postProcessCopyShader = nullptr;
}

void Renderer::BeginFrame()
{
	if (m_renderBackend != nullptr)
	{
		m_renderBackend->BeginFrame();
	}
}

void Renderer::EndFrame()
{
	if (m_renderBackend != nullptr)
	{
		m_renderBackend->EndFrame();
	}
}

void Renderer::CreateRenderingContext() { m_renderBackend->CreateRenderingContext(); }

void Renderer::SubmitRenderRequest(RenderRequestInfo const& request)
{
	auto& requestList = m_renderRequests[request.m_pass];
	requestList.push_back(request);
}

void Renderer::ClearRenderRequests()
{
	for (auto& pair : m_renderRequests)
	{
		pair.second.clear();
	}
}

void Renderer::RenderViewport(Viewport const& viewport)
{
	EnsureViewport(viewport);
	RenderOpaque(viewport);
	RenderSkybox(viewport);
	RenderPostProcess(viewport);
	RenderUI(viewport);

	ClearRenderRequests();
}

void Renderer::RenderRequest(RenderRequestInfo const& request)
{
	m_renderBackend->BindShader(request.m_shader);
	m_renderBackend->BindTexture(request.m_diffuseTexture);
	m_renderBackend->BindModelConstants(request.m_modelToWorld, request.m_tint);
	m_renderBackend->SetBlendMode(request.m_blendMode);
	m_renderBackend->SetRasterizerMode(request.m_rasterizerMode);
	m_renderBackend->SetDepthMode(request.m_depthMode);

	if (request.m_indexBuffer != nullptr)
	{
		m_renderBackend->DrawIndexedVertexBuffer(request.m_vertexBuffer, request.m_indexBuffer);
	}
	else if (request.m_vertexBuffer != nullptr)
	{
		m_renderBackend->DrawVertexBuffer(request.m_vertexBuffer);
	}
}

void Renderer::EnsureViewport(Viewport const& viewport)
{
	ResizeSceneTargets(viewport.m_outputResolution);
	m_postProcessChain->Resize(*m_renderBackend, viewport.m_outputResolution);

	IntVec2 topLeft       = (IntVec2)viewport.m_outputRect.m_mins;
	IntVec2 rectDimension = (IntVec2)viewport.m_outputRect.GetDimensions();
	m_renderBackend->SetViewport(rectDimension, topLeft);
}

void Renderer::ResizeSceneTargets(IntVec2 dimensions)
{
	if (dimensions.x <= 0 || dimensions.y <= 0)
	{
		return;
	}

	if (dimensions == m_sceneTargetDimensions && m_sceneColorTexture != nullptr && m_sceneDepthTexture != nullptr
		&& m_sceneNormalTexture != nullptr)
	{
		return;
	}

	DestroySceneTargets();

	m_sceneColorTexture      = m_renderBackend->CreateRenderTargetTexture("SceneColor", dimensions);
	m_sceneDepthTexture      = m_renderBackend->CreateDepthStencilTexture("SceneDepth", dimensions);
	m_sceneNormalTexture     = m_renderBackend->CreateRenderTargetTexture("SceneNormal", dimensions);
	m_sceneTargetDimensions = dimensions;

	m_renderBackend->ClearRenderTarget(m_sceneNormalTexture, Rgba8(128, 128, 128, 255));
	m_renderBackend->ClearDepthStencil(m_sceneDepthTexture);
}

void Renderer::DestroySceneTargets()
{
	if (m_renderBackend == nullptr)
	{
		m_sceneColorTexture      = nullptr;
		m_sceneDepthTexture      = nullptr;
		m_sceneNormalTexture     = nullptr;
		m_sceneTargetDimensions = IntVec2::Zero;
		return;
	}

	m_renderBackend->DestroyTexture(m_sceneColorTexture);
	m_renderBackend->DestroyTexture(m_sceneDepthTexture);
	m_renderBackend->DestroyTexture(m_sceneNormalTexture);
	m_sceneTargetDimensions = IntVec2::Zero;
}

void Renderer::ClearSceneTargets(Rgba8 const& clearColor)
{
	m_renderBackend->ClearRenderTarget(m_sceneColorTexture, clearColor);
	m_renderBackend->ClearRenderTarget(m_sceneNormalTexture, Rgba8(128, 128, 128, 255));
	m_renderBackend->ClearDepthStencil(m_sceneDepthTexture);
}

void Renderer::BindSceneTargets(bool bindNormal)
{
	Texture* normalTarget = bindNormal ? m_sceneNormalTexture : nullptr;
	m_renderBackend->BindRenderTargets(m_sceneColorTexture, m_sceneDepthTexture, normalTarget);
}

void Renderer::CopyTextureToBackBuffer(Texture* colorTexture)
{
	if (colorTexture == nullptr)
	{
		return;
	}

	m_renderBackend->BindBackBuffer();
	m_renderBackend->BindPostProcessInputs(colorTexture, nullptr, nullptr);
	m_renderBackend->DrawFullscreenTriangle(m_postProcessCopyShader, L"FinalCopyToBackBuffer");
	m_renderBackend->UnbindAllShaderResourceViews();
}

void Renderer::RenderOpaque(Viewport const& viewport)
{
	if (viewport.m_worldCamera == nullptr)
	{
		return;
	}

	BindSceneTargets(true);

	m_renderBackend->SetBlendMode(BlendMode::ALPHA);
	m_renderBackend->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	m_renderBackend->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	m_renderBackend->BeginCamera(*viewport.m_worldCamera);

	for (RenderRequestInfo const& request : m_renderRequests[RenderRequestPass::Opaque])
	{
		RenderRequest(request);
	}

	DebugRenderWorld(*viewport.m_worldCamera);
}

void Renderer::RenderSkybox(Viewport const& viewport)
{
	if (viewport.m_worldCamera == nullptr)
	{
		return;
	}

	BindSceneTargets(false);

	m_renderBackend->SetBlendMode(BlendMode::ALPHA);
	m_renderBackend->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	m_renderBackend->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	for (RenderRequestInfo const& request : m_renderRequests[RenderRequestPass::Skybox])
	{
		RenderRequest(request);
	}
}

void Renderer::RenderPostProcess(Viewport const& viewport)
{
	if (viewport.m_worldCamera == nullptr)
	{
		return;
	}

	PostProcessContext context;
	context.m_camera           = viewport.m_worldCamera;
	context.m_sceneColor       = m_sceneColorTexture;
	context.m_sceneDepth       = m_sceneDepthTexture;
	context.m_sceneNormal      = m_sceneNormalTexture;
	context.m_outputResolution = viewport.m_outputResolution;

	Texture* finalColor = m_postProcessChain->Render(*m_renderBackend, context);
	CopyTextureToBackBuffer(finalColor);
}

void Renderer::RenderUI(Viewport const& viewport)
{
	if (viewport.m_uiCamera == nullptr)
	{
		return;
	}

	m_renderBackend->BindBackBuffer();
	m_renderBackend->SetBlendMode(BlendMode::ALPHA);
	m_renderBackend->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	m_renderBackend->SetDepthMode(DepthMode::READ_ONLY_ALWAYS);
	m_renderBackend->BeginCamera(*viewport.m_uiCamera);

	for (RenderRequestInfo const& request : m_renderRequests[RenderRequestPass::UI])
	{
		RenderRequest(request);
	}

	DebugRenderScreen(*viewport.m_uiCamera);
}
void Renderer::BeginCamera(Camera const& camera) { m_renderBackend->BeginCamera(camera); }
void Renderer::EndCamera() { m_renderBackend->EndCamera(); }
void Renderer::ClearScreen(Rgba8 const& clearColor)
{
	m_renderBackend->ClearScreen(clearColor);
	ClearSceneTargets(clearColor);
}
void Renderer::SetBlendMode(BlendMode blendMode) { m_renderBackend->SetBlendMode(blendMode); }
void Renderer::SetRasterizerMode(RasterizerMode rasterizerMode) { m_renderBackend->SetRasterizerMode(rasterizerMode); }
void Renderer::SetDepthMode(DepthMode depthMode) { m_renderBackend->SetDepthMode(depthMode); }
void Renderer::SetStatesIfChanged() { m_renderBackend->SetStatesIfChanged(); }

void Renderer::DrawVertexArray(int numVertexes, Vertex const* vertexes)
{
	m_renderBackend->DrawVertexArray(numVertexes, vertexes);
}
void Renderer::DrawVertexArray(std::vector<Vertex> const& verts) { m_renderBackend->DrawVertexArray(verts); }
void Renderer::DrawVertexArray(std::vector<Vertex> const& verts, std::vector<unsigned int> const& vertIndexs)
{
	m_renderBackend->DrawVertexArray(verts, vertIndexs);
}
void Renderer::DrawVertexBuffer(VertexBuffer* vertexBuffer) { m_renderBackend->DrawVertexBuffer(vertexBuffer); }
void Renderer::DrawIndexedVertexBuffer(VertexBuffer* vertexBuffer, IndexBuffer* indexBuffer)
{
	m_renderBackend->DrawIndexedVertexBuffer(vertexBuffer, indexBuffer);
}

void Renderer::BindTexture(Texture* textureOrNull) { m_renderBackend->BindTexture(textureOrNull); }
void Renderer::BindTexture(Texture* textureOrNull, unsigned int slot)
{
	m_renderBackend->BindTexture(textureOrNull, slot);
}
void Renderer::BindSampler(SamplerMode samplerMode, unsigned int slot)
{
	m_renderBackend->BindSampler(samplerMode, slot);
}
void Renderer::BindShader(Shader* shader) { m_renderBackend->BindShader(shader); }
void Renderer::BindModelConstants(Matrix4x4 const& modelToWorldTransform, Rgba8 const& modelColor)
{
	m_renderBackend->BindModelConstants(modelToWorldTransform, modelColor);
}
void Renderer::BindLightConstants(LightConstants const& lightConstants)
{
	m_renderBackend->BindLightConstants(lightConstants);
}
void Renderer::BindLightConstants(
	Vec3 const& sunDirection, float sunIntensity, Rgba8 const& ambientColor, float ambientIntensity
)
{
	m_renderBackend->BindLightConstants(sunDirection, sunIntensity, ambientColor, ambientIntensity);
}
void Renderer::BindPostProcessConstants(Vec2 const& screenDimensions, float cameraNear, float cameraFar)
{
	m_renderBackend->BindPostProcessConstants(screenDimensions, cameraNear, cameraFar);
}
void Renderer::BindFrameConstants(float time, float deltaSeconds)
{
	m_renderBackend->BindFrameConstants(time, deltaSeconds);
}

Shader*  Renderer::CreateOrGetShader(char const* shaderName) { return m_renderBackend->CreateOrGetShader(shaderName); }
Texture* Renderer::CreateOrGetTexture(char const* fileDataPath)
{
	return m_renderBackend->CreateOrGetTexture(fileDataPath);
}
Texture* Renderer::CreateTextureFromImage(const Image& image) { return m_renderBackend->CreateTextureFromImage(image); }
Texture* Renderer::CreateTextureFromData(char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t* texelData)
{
	return m_renderBackend->CreateTextureFromData(name, dimensions, bytesPerTexel, texelData);
}
Texture* Renderer::CreateRenderTargetTexture(char const* name, IntVec2 dimensions)
{
	return m_renderBackend->CreateRenderTargetTexture(name, dimensions);
}
Texture* Renderer::CreateDepthStencilTexture(char const* name, IntVec2 dimensions)
{
	return m_renderBackend->CreateDepthStencilTexture(name, dimensions);
}
BitmapFont* Renderer::CreateOrGetBitmapFont(char const* fontFilePathNameWithNoExtension)
{
	return m_renderBackend->CreateOrGetBitmapFont(fontFilePathNameWithNoExtension);
}

VertexBuffer* Renderer::CreateVertexBuffer(const unsigned int size, unsigned int stride)
{
	return m_renderBackend->CreateVertexBuffer(size, stride);
}
VertexBuffer* Renderer::CreateVertexBuffer(std::vector<Vertex> const& verts)
{
	return m_renderBackend->CreateVertexBuffer(verts);
}
ConstantBuffer* Renderer::CreateConstantBuffer(const unsigned int size)
{
	return m_renderBackend->CreateConstantBuffer(size);
}
IndexBuffer* Renderer::CreateIndexBuffer(const unsigned int size) { return m_renderBackend->CreateIndexBuffer(size); }
IndexBuffer* Renderer::CreateIndexBuffer(std::vector<unsigned int> const& indexes)
{
	return m_renderBackend->CreateIndexBuffer(indexes);
}

void Renderer::CopyCPUToGPU(const void* data, unsigned int size, VertexBuffer* vertexBuffer)
{
	m_renderBackend->CopyCPUToGPU(data, size, vertexBuffer);
}
void Renderer::CopyCPUToGPU(const void* data, unsigned int size, ConstantBuffer* constantBuffer)
{
	m_renderBackend->CopyCPUToGPU(data, size, constantBuffer);
}
void Renderer::CopyCPUToGPU(const void* data, unsigned int size, IndexBuffer* indexBuffer)
{
	m_renderBackend->CopyCPUToGPU(data, size, indexBuffer);
}

void                 Renderer::BeginEvent(std::string const& eventName) { m_renderBackend->BeginEvent(eventName); }
void                 Renderer::EndEvent() { m_renderBackend->EndEvent(); }
ID3D11Device*        Renderer::GetD3DDevice() const { return m_renderBackend->GetD3DDevice(); }
ID3D11DeviceContext* Renderer::GetD3DDeviceContext() const { return m_renderBackend->GetD3DDeviceContext(); }

void Renderer::SetViewport(IntVec2 dimensions, IntVec2 topLeft) { m_renderBackend->SetViewport(dimensions, topLeft); }
void Renderer::ResizeBackBuffer(IntVec2 newDimensions) { m_renderBackend->ResizeBackBuffer(newDimensions); }
Texture* Renderer::GetTextureFromFileName(char const* fileName)
{
	return m_renderBackend->GetTextureFromFileName(fileName);
}