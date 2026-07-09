#include "MingEngine/Engine/Render/Renderer.hpp"

#include "MingEngine/Core/Clock.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"
#include "MingEngine/Engine/Render/PostProcessChain.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

#include "ThirdParty/imgui/backends/imgui_impl_dx11.h"

Renderer::Renderer(RendererConfig config) : m_config(config) {}

Renderer::~Renderer() {}

void Renderer::BindMethods() {}

void Renderer::Startup()
{
	if (m_renderBackend != nullptr)
	{
		return;
	}

	m_renderBackend = new D3D11RenderBackend(m_config);
	m_renderBackend->Startup();
	m_postProcessCopyShader = m_renderBackend->CreateOrGetShader("res://Shaders/PostProcessCopy.hlsl");
}

void Renderer::Shutdown()
{
	if (m_renderBackend != nullptr)
	{
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

void Renderer::RenderViewport(ViewportInfo& viewport)
{
	// 1) Ensure this Viewport owns correctly sized targets.
	// 2) Render world passes and post process into the output texture.
	// 3) Render UI on top without presenting to the back buffer here.
	if (viewport.m_worldCamera == nullptr || viewport.m_viewportOutputTexture == nullptr)
	{
		return;
	}

	m_renderBackend->BindCamera(*viewport.m_worldCamera);
	PrepareConstants(viewport);

	RenderOpaque(viewport);
	RenderSkybox(viewport);
	RenderPostProcess(viewport);

	RenderUI(viewport);
}

void Renderer::ExecuteRenderRequest(RenderRequest const& request)
{
	ModelConstants modelData  = ModelConstants();
	modelData.m_modelToWorld  = request.m_modelToWorld;
	modelData.m_modelColor[0] = request.m_tint.r / 255.f;
	modelData.m_modelColor[1] = request.m_tint.g / 255.f;
	modelData.m_modelColor[2] = request.m_tint.b / 255.f;
	modelData.m_modelColor[3] = request.m_tint.a / 255.f;
	m_renderBackend->UpdateAndBindConstantBuffer(BuiltinConstantBufferType::Model, modelData);

	m_renderBackend->BindShader(request.m_shader);
	for (unsigned int textureSlot = 0; textureSlot < request.m_textures.size(); ++textureSlot)
	{
		m_renderBackend->BindTexture(request.m_textures[textureSlot], textureSlot);
		m_renderBackend->BindSampler(request.m_samplerMode, textureSlot);
	}
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

void Renderer::ResizeViewport(ViewportInfo& viewport, IntVec2 dimensions)
{
	if (dimensions.x <= 0 || dimensions.y <= 0)
	{
		return;
	}

	// Resize is destructive because all targets must keep matching dimensions.
	// Callers must not retain texture or SRV pointers across this operation.
	DestroyViewportResources(viewport);

	viewport.m_outputResolution      = dimensions;
	viewport.m_outputRect            = AABB2(Vec2::Zero, (Vec2)dimensions);
	viewport.m_viewportOutputTexture = m_renderBackend->CreateRenderTargetTexture("ViewportOutput", dimensions);
	viewport.m_sceneColorTexture     = m_renderBackend->CreateRenderTargetTexture("SceneColor", dimensions);
	viewport.m_sceneDepthTexture     = m_renderBackend->CreateDepthStencilTexture("SceneDepth", dimensions);
	viewport.m_sceneNormalTexture    = m_renderBackend->CreateRenderTargetTexture("SceneNormal", dimensions);
	viewport.m_pingTexture           = m_renderBackend->CreateRenderTargetTexture("Ping", dimensions);
	viewport.m_pongTexture           = m_renderBackend->CreateRenderTargetTexture("Pong", dimensions);

	m_renderBackend->ClearRenderTarget(viewport.m_sceneNormalTexture, Rgba8(128, 128, 128, 255));
	m_renderBackend->ClearDepthStencil(viewport.m_sceneDepthTexture);
}

void Renderer::DestroyViewportResources(ViewportInfo& viewport)
{
	// Keep destruction centralized so every raw pointer is cleared immediately.
	GPUTexture** textures[] = {
		&viewport.m_viewportOutputTexture, &viewport.m_sceneColorTexture, &viewport.m_sceneDepthTexture,
		&viewport.m_sceneNormalTexture,    &viewport.m_pingTexture,       &viewport.m_pongTexture,
	};

	for (GPUTexture** texture : textures)
	{
		if (*texture != nullptr)
		{
			m_renderBackend->DestroyTexture(*texture);
			*texture = nullptr;
		}
	}
}

void Renderer::SetViewport(IntVec2 dimensions, IntVec2 topLeft) { m_renderBackend->SetViewport(dimensions, topLeft); }

void Renderer::ClearSceneTargets(ViewportInfo const& viewport)
{
	m_renderBackend->ClearRenderTarget(viewport.m_viewportOutputTexture, viewport.m_clearColor);
	m_renderBackend->ClearRenderTarget(viewport.m_sceneColorTexture, viewport.m_clearColor);
	m_renderBackend->ClearRenderTarget(viewport.m_sceneNormalTexture, Rgba8(128, 128, 128, 255));
	m_renderBackend->ClearDepthStencil(viewport.m_sceneDepthTexture);
}

void Renderer::CopyTextureToBackBuffer(GPUTexture* colorTexture)
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

void Renderer::PrepareConstants(ViewportInfo const& viewport)
{
	// Prepare light constants
	LightConstants lightConstants  = LightConstants();
	int            pointLightCount = 0;
	for (LightInfo const& light : viewport.m_lights)
	{
		switch (light.m_type)
		{
		case LightType::DIRECTIONAL:
			lightConstants.m_directionalLight.m_direction = light.m_direction;
			lightConstants.m_directionalLight.m_intensity = light.m_intensity;
			break;
		case LightType::POINT:
			if (pointLightCount < kMaxPointLights)
			{
				Vec3 gpuColor;
				gpuColor.x = light.m_color.r / 255.f;
				gpuColor.y = light.m_color.g / 255.f;
				gpuColor.z = light.m_color.b / 255.f;

				lightConstants.m_pointLights[pointLightCount].m_position  = light.m_position;
				lightConstants.m_pointLights[pointLightCount].m_intensity = light.m_intensity;
				lightConstants.m_pointLights[pointLightCount].m_color     = gpuColor;
				lightConstants.m_pointLights[pointLightCount].m_range     = light.m_range;
				++pointLightCount;
			}
			break;
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
	frameConstants.m_time         = (float)systemClock.GetTotalSeconds();
	frameConstants.m_deltaSeconds = (float)systemClock.GetDeltaSeconds();
	m_renderBackend->UpdateAndBindConstantBuffer(BuiltinConstantBufferType::Frame, frameConstants);
}

void Renderer::RenderOpaque(ViewportInfo& viewport)
{
	if (viewport.m_worldCamera == nullptr)
	{
		return;
	}

	m_renderBackend->BindRenderTargets(
		viewport.m_sceneColorTexture,
		viewport.m_sceneDepthTexture,
		viewport.m_sceneNormalTexture);

	m_renderBackend->SetBlendMode(BlendMode::ALPHA);
	m_renderBackend->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	m_renderBackend->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	DebugRenderWorld(*viewport.m_worldCamera, viewport);

	for (RenderRequest const& request : viewport.m_renderRequests[(int)RenderRequestPass::Opaque])
	{
		ExecuteRenderRequest(request);
	}
}

void Renderer::RenderSkybox(ViewportInfo const& viewport)
{
	if (viewport.m_worldCamera == nullptr)
	{
		return;
	}

	m_renderBackend->BindRenderTargets(viewport.m_sceneColorTexture, viewport.m_sceneDepthTexture, nullptr);

	m_renderBackend->SetBlendMode(BlendMode::ALPHA);
	m_renderBackend->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	m_renderBackend->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	for (RenderRequest const& request : viewport.m_renderRequests[(int)RenderRequestPass::Skybox])
	{
		ExecuteRenderRequest(request);
	}
}

void Renderer::RenderPostProcess(ViewportInfo& viewport)
{
	if (viewport.m_worldCamera == nullptr)
	{
		return;
	}

	PostProcessContext context;
	context.m_camera           = viewport.m_worldCamera;
	context.m_sceneColor       = viewport.m_sceneColorTexture;
	context.m_sceneDepth       = viewport.m_sceneDepthTexture;
	context.m_sceneNormal      = viewport.m_sceneNormalTexture;
	context.m_ping             = viewport.m_pingTexture;
	context.m_pong             = viewport.m_pongTexture;
	context.m_outputResolution = viewport.m_outputResolution;

	GPUTexture* finalColor = viewport.m_postProcessChain.Render(*m_renderBackend, context);

	m_renderBackend->BindRenderTarget(viewport.m_viewportOutputTexture);
	m_renderBackend->BindPostProcessInputs(finalColor, viewport.m_sceneDepthTexture, viewport.m_sceneNormalTexture);
	m_renderBackend->DrawFullscreenTriangle(m_postProcessCopyShader, L"CopyPostProcessToOutput");
	m_renderBackend->UnbindAllShaderResourceViews();
}

void Renderer::RenderUI(ViewportInfo const& viewport)
{
	CameraContext uiCameraData = CameraContext();
	uiCameraData.SetOrthogonal(Vec2::Zero, (Vec2)viewport.m_outputResolution, 0.f, 1.f);
	m_renderBackend->BindCamera(uiCameraData);

	m_renderBackend->BindRenderTarget(viewport.m_viewportOutputTexture);
	m_renderBackend->SetBlendMode(BlendMode::ALPHA);
	m_renderBackend->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	m_renderBackend->SetDepthMode(DepthMode::READ_ONLY_ALWAYS);

	std::vector<RenderRequest> const& uiRequests = viewport.m_renderRequests[(int)RenderRequestPass::UI];
	for (size_t requestIndex = 0; requestIndex < uiRequests.size(); ++requestIndex)
	{
		ExecuteRenderRequest(uiRequests[requestIndex]);
	}
}

Shader* Renderer::CreateOrGetShader(std::string const& shaderVirtualPath)
{
	return m_renderBackend->CreateOrGetShader(shaderVirtualPath);
}
GPUTexture* Renderer::CreateGPUTexture(
	char const* name, IntVec2 dimensions, int bytesPerTexel, uint8_t const* texelData)
{
	return m_renderBackend->CreateGPUTexture(name, dimensions, bytesPerTexel, texelData);
}
GPUTexture* Renderer::CreateRenderTargetTexture(char const* name, IntVec2 dimensions)
{
	return m_renderBackend->CreateRenderTargetTexture(name, dimensions);
}
GPUTexture* Renderer::CreateDepthStencilTexture(char const* name, IntVec2 dimensions)
{
	return m_renderBackend->CreateDepthStencilTexture(name, dimensions);
}

void Renderer::DestroyTexture(GPUTexture* texture)
{
	if (m_renderBackend != nullptr)
	{
		m_renderBackend->DestroyTexture(texture);
	}
}

VertexBuffer* Renderer::CreateVertexBuffer(const unsigned int size, unsigned int stride)
{
	return m_renderBackend->CreateVertexBuffer(size, stride);
}
VertexBuffer* Renderer::CreateVertexBuffer(void const* data, const unsigned int byteSize, unsigned int stride)
{
	return m_renderBackend->CreateVertexBuffer(data, byteSize, stride);
}
ConstantBuffer* Renderer::CreateConstantBuffer(const unsigned int size)
{
	return m_renderBackend->CreateConstantBuffer(size);
}
IndexBuffer* Renderer::CreateIndexBuffer(const unsigned int size) { return m_renderBackend->CreateIndexBuffer(size); }
IndexBuffer* Renderer::CreateIndexBuffer(void const* data, const unsigned int byteSize, const unsigned int stride)
{
	return m_renderBackend->CreateIndexBuffer(data, byteSize, stride);
}

void Renderer::UpdateVertexBuffer(VertexBuffer* vertexBuffer, void const* data, unsigned int byteSize)
{
	if (vertexBuffer == nullptr)
	{
		return;
	}

	vertexBuffer->Resize(byteSize);
	CopyCPUToGPU(data, byteSize, vertexBuffer);
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

void Renderer::BindConstantBuffer(ConstantBuffer* constantBuffer, int slot)
{
	m_renderBackend->BindConstantBuffer(constantBuffer, slot);
}

void Renderer::InitImGuiD3D11Backend()
{
	ImGui_ImplDX11_Init(m_renderBackend->GetD3DDevice(), m_renderBackend->GetD3DDeviceContext());
}

void Renderer::BindBackBuffer() { m_renderBackend->BindBackBuffer(); }

void Renderer::ResizeBackBuffer(IntVec2 newDimensions) { m_renderBackend->ResizeBackBuffer(newDimensions); }
