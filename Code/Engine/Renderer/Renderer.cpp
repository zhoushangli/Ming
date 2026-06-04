#include "Engine/Renderer/Renderer.hpp"

#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/DebugRenderer.hpp"
#include "Engine/Renderer/PostProcessChain.hpp"
#include "Renderer.hpp"

const LightHandle LightHandle::Invalid = LightHandle{ static_cast<size_t>(-1) };

Renderer::Renderer(RendererConfig config) : m_config(config) {}

Renderer::~Renderer() {}

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
		m_sceneColorTexture  = nullptr;
		m_sceneDepthTexture  = nullptr;
		m_sceneNormalTexture = nullptr;

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

void Renderer::SubmitRenderRequest(RenderRequest const& request)
{
	auto& requestList = m_renderRequests[(int)request.m_pass];
	requestList.push_back(request);
}

void Renderer::ClearRenderRequests()
{
	for (auto& requestList : m_renderRequests)
	{
		requestList.clear();
	}
}

LightHandle Renderer::RegisterLight()
{
	for (size_t i = 0; i < m_lights.size(); ++i)
	{
		if (!m_lights[i].m_isActive)
		{
			m_lights[i].m_isActive = true;
			return LightHandle{ i };
		}
	}

	LightInfo newLight;
	newLight.m_isActive = true;
	m_lights.push_back(newLight);

	return LightHandle{ m_lights.size() - 1 };
}

void Renderer::UnregisterLight(LightHandle handle)
{
	if (handle.IsValid() && handle.m_index < m_lights.size())
	{
		m_lights[handle.m_index].m_isActive = false;
	}
}

void Renderer::UpdateLight(LightHandle handle, LightInfo const& info)
{
	if (handle.IsValid() && handle.m_index < m_lights.size())
	{
		m_lights[handle.m_index] = info;
	}
}

void Renderer::RenderViewport(Viewport const& viewport)
{
	EnsureViewport(viewport);
	m_renderBackend->ClearScreen(viewport.m_clearColor);
	ClearSceneTargets(viewport.m_clearColor);

	m_renderBackend->BindCamera(*viewport.m_worldCamera);
	PrepareConstants(viewport);

	RenderOpaque(viewport);
	RenderSkybox(viewport);
	RenderPostProcess(viewport);

	m_renderBackend->BindCamera(*viewport.m_uiCamera);
	RenderUI(viewport);

	ClearRenderRequests();
}

void Renderer::ExecuteRenderRequest(RenderRequest const& request)
{
	ModelConstants modelData = ModelConstants();
	modelData.ModelToWorld   = request.m_modelToWorld;
	modelData.ModelColor[0]  = request.m_tint.r / 255.f;
	modelData.ModelColor[1]  = request.m_tint.g / 255.f;
	modelData.ModelColor[2]  = request.m_tint.b / 255.f;
	modelData.ModelColor[3]  = request.m_tint.a / 255.f;
	m_renderBackend->UpdateAndBindConstantBuffer(BuiltinConstantBufferType::Model, modelData);

	m_renderBackend->BindShader(request.m_shader);
	m_renderBackend->BindTexture(request.m_diffuseTexture);
	m_renderBackend->BindSampler(request.m_samplerMode);
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

	// TODO: Should complete destroy logic
	// DestroySceneTargets();

	m_sceneColorTexture     = m_renderBackend->CreateRenderTargetTexture("SceneColor", dimensions);
	m_sceneDepthTexture     = m_renderBackend->CreateDepthStencilTexture("SceneDepth", dimensions);
	m_sceneNormalTexture    = m_renderBackend->CreateRenderTargetTexture("SceneNormal", dimensions);
	m_sceneTargetDimensions = dimensions;

	m_renderBackend->ClearRenderTarget(m_sceneNormalTexture, Rgba8(128, 128, 128, 255));
	m_renderBackend->ClearDepthStencil(m_sceneDepthTexture);
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

void Renderer::PrepareConstants(Viewport const& viewport)
{
	// Prepare light constants
	LightConstants lightConstants  = LightConstants();
	int            pointLightCount = 0;
	for (size_t i = 0; i < m_lights.size(); ++i)
	{
		if (m_lights[i].m_isActive)
		{
			switch (m_lights[i].m_type)
			{
			case LightType::DIRECTIONAL:
				lightConstants.m_directionalLight.m_direction = m_lights[i].m_direction;
				lightConstants.m_directionalLight.m_intensity = m_lights[i].m_intensity;
				break;
			case LightType::POINT:
				if (pointLightCount < kMaxPointLights)
				{
					Vec3 gpuColor;
					gpuColor.x = m_lights[i].m_color.r / 255.f;
					gpuColor.y = m_lights[i].m_color.g / 255.f;
					gpuColor.z = m_lights[i].m_color.b / 255.f;

					lightConstants.m_pointLights[pointLightCount].m_position  = m_lights[i].m_position;
					lightConstants.m_pointLights[pointLightCount].m_intensity = m_lights[i].m_intensity;
					lightConstants.m_pointLights[pointLightCount].m_color     = gpuColor;
					lightConstants.m_pointLights[pointLightCount].m_range     = m_lights[i].m_range;
					++pointLightCount;
				}
				break;
			}
		}
	}
	lightConstants.m_pointLightCount = pointLightCount;
	m_renderBackend->UpdateAndBindConstantBuffer(BuiltinConstantBufferType::Light, lightConstants);

	// Prepare post-process constants
	PostProcessConstants postProcessConstants;
	postProcessConstants.m_screenDimensions = (Vec2)viewport.m_outputResolution;
	postProcessConstants.m_cameraNear       = viewport.m_worldCamera->GetNearZ();
	postProcessConstants.m_cameraFar        = viewport.m_worldCamera->GetFarZ();
	m_renderBackend->UpdateAndBindConstantBuffer(BuiltinConstantBufferType::PostProcess, postProcessConstants);

	// Prepare frame constants
	// TODO: Actually the renderer should not be responsible for tracking time
	// this should be passed in from the game or engine layer
	FrameConstants frameConstants;
	Clock&         systemClock    = Clock::GetSystemClock();
	frameConstants.m_time         = systemClock.GetTotalSeconds();
	frameConstants.m_deltaSeconds = systemClock.GetDeltaSeconds();
	m_renderBackend->UpdateAndBindConstantBuffer(BuiltinConstantBufferType::Frame, frameConstants);
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

	DebugRenderWorld(*viewport.m_worldCamera);

	for (RenderRequest const& request : m_renderRequests[(int)RenderRequestPass::Opaque])
	{
		ExecuteRenderRequest(request);
	}
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

	for (RenderRequest const& request : m_renderRequests[(int)RenderRequestPass::Skybox])
	{
		ExecuteRenderRequest(request);
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
	m_renderBackend->BindCamera(*viewport.m_uiCamera);

	size_t const existingUIRequestCount = m_renderRequests[(int)RenderRequestPass::UI].size();
	DebugRenderScreen(*viewport.m_uiCamera);

	std::vector<RenderRequest> const& uiRequests = m_renderRequests[(int)RenderRequestPass::UI];
	for (size_t requestIndex = existingUIRequestCount; requestIndex < uiRequests.size(); ++requestIndex)
	{
		ExecuteRenderRequest(uiRequests[requestIndex]);
	}

	for (size_t requestIndex = 0; requestIndex < existingUIRequestCount; ++requestIndex)
	{
		ExecuteRenderRequest(uiRequests[requestIndex]);
	}
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
