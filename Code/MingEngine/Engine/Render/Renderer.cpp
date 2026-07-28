#include "MingEngine/Engine/Render/Renderer.hpp"

#include "MingEngine/Core/Clock.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"
#include "MingEngine/Engine/Render/PostProcessChain.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

#include "ThirdParty/imgui/backends/imgui_impl_dx11.h"

#include <algorithm>

namespace
{

// clang-format off
const uint8_t kDefaultWhiteTexture[16] =
{
	0xFF, 0xFF, 0xFF, 0xFF, // (0,0)
	0xFF, 0xFF, 0xFF, 0xFF, // (1,0)
	0xFF, 0xFF, 0xFF, 0xFF, // (0,1)
	0xFF, 0xFF, 0xFF, 0xFF  // (1,1)
};

const uint8_t kDefaultMagentaTexture[16] =
{
	0xFF, 0x00, 0xFF, 0xFF, // (0,0)
	0xFF, 0x00, 0xFF, 0xFF, // (1,0)
	0xFF, 0x00, 0xFF, 0xFF, // (0,1)
	0xFF, 0x00, 0xFF, 0xFF  // (1,1)
};

const uint8_t kDefaultNormalTexture[16] =
{
	0x80, 0x80, 0xFF, 0xFF, // (0,0)
	0x80, 0x80, 0xFF, 0xFF, // (1,0)
	0x80, 0x80, 0xFF, 0xFF, // (0,1)
	0x80, 0x80, 0xFF, 0xFF  // (1,1)
};

const uint8_t kDefaultSGETexture[16] =
{
	0x80, 0x80, 0x00, 0xFF, // (0,0)
	0x80, 0x80, 0x00, 0xFF, // (1,0)
	0x80, 0x80, 0x00, 0xFF, // (0,1)
	0x80, 0x80, 0x00, 0xFF  // (1,1)
};
// clang-format on

} // namespace

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
	m_defaultWhiteTexture   = CreateGPUTexture("DefaultWhite", IntVec2(2, 2), 4, kDefaultWhiteTexture);
	m_defaultMagentaTexture = CreateGPUTexture("DefaultMagenta", IntVec2(2, 2), 4, kDefaultMagentaTexture);
	m_defaultNormalTexture  = CreateGPUTexture("DefaultNormal", IntVec2(2, 2), 4, kDefaultNormalTexture);
	m_defaultSGETexture     = CreateGPUTexture("DefaultSGE", IntVec2(2, 2), 4, kDefaultSGETexture);
	m_defaultShaderResource         = GetBuiltinShaderResource("DefaultUnlit", BuiltinShaders::DefaultUnlit);
	m_postProcessCopyShaderResource =
		GetBuiltinShaderResource("PostProcessCopy", BuiltinShaders::PostProcessCopy);

	DebugRenderConfig debugConfig;
	debugConfig.m_renderer = this;
	DebugGizmos::Startup(debugConfig);
}

void Renderer::Shutdown()
{
	DebugGizmos::Shutdown();
	m_defaultShaderResource         = nullptr;
	m_postProcessCopyShaderResource = nullptr;
	m_builtinShaderResources.clear();

	if (m_renderBackend != nullptr)
	{
		DestroyTexture(m_defaultWhiteTexture);
		DestroyTexture(m_defaultMagentaTexture);
		DestroyTexture(m_defaultNormalTexture);
		DestroyTexture(m_defaultSGETexture);
		m_defaultWhiteTexture   = nullptr;
		m_defaultMagentaTexture = nullptr;
		m_defaultNormalTexture  = nullptr;
		m_defaultSGETexture     = nullptr;

		m_renderBackend->Shutdown();
		delete m_renderBackend;
		m_renderBackend = nullptr;
	}
}

void Renderer::BeginFrame()
{
	DebugGizmos::BeginFrame();

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

	DebugGizmos::EndFrame();
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
	DebugGizmos::PrepareRenderRequests();

	auto sortPass = [](std::vector<RenderRequest>& requests)
	{
		std::stable_sort(
			requests.begin(),
			requests.end(),
			[](RenderRequest const& a, RenderRequest const& b) { return a.m_renderPriority < b.m_renderPriority; });
	};
	sortPass(viewport.m_renderRequests[static_cast<size_t>(RenderRequestPass::Skybox)]);
	sortPass(viewport.m_renderRequests[static_cast<size_t>(RenderRequestPass::Opaque)]);
	sortPass(viewport.m_renderRequests[static_cast<size_t>(RenderRequestPass::Transparent)]);
	sortPass(viewport.m_renderRequests[static_cast<size_t>(RenderRequestPass::UI)]);

	RenderSkybox(viewport);
	RenderOpaque(viewport);
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

	Shader* shader = request.m_shader;
	if (shader == nullptr && m_defaultShaderResource.IsValid())
	{
		shader = m_defaultShaderResource->GetShader();
	}
	m_renderBackend->BindShader(shader);
	for (unsigned int textureSlot = 0; textureSlot < request.m_textures.size(); ++textureSlot)
	{
		GPUTexture* texture = request.m_textures[textureSlot];
		if (texture == nullptr)
		{
			switch (textureSlot)
			{
			case SurfaceTextureSlot::Diffuse:
				m_renderBackend->BindTexture(m_defaultWhiteTexture, textureSlot);
				break;
			case SurfaceTextureSlot::Normal:
				m_renderBackend->BindTexture(m_defaultNormalTexture, textureSlot);
				break;
			case SurfaceTextureSlot::SGE:
				m_renderBackend->BindTexture(m_defaultSGETexture, textureSlot);
				break;
			default:
				m_renderBackend->BindTexture(m_defaultWhiteTexture, textureSlot);
				break;
			}
			m_renderBackend->BindSampler(SamplerMode::POINT_CLAMP, textureSlot);
		}
		else
		{
			m_renderBackend->BindTexture(request.m_textures[textureSlot], textureSlot);
			m_renderBackend->BindSampler(request.m_samplerMode, textureSlot);
		}
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

	m_renderBackend->ClearRenderTarget(viewport.m_sceneNormalTexture, Color(128, 128, 128, 255));
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
	m_renderBackend->ClearRenderTarget(viewport.m_sceneNormalTexture, Color(128, 128, 128, 255));
	m_renderBackend->ClearDepthStencil(viewport.m_sceneDepthTexture);
}

void Renderer::CopyTextureToBackBuffer(GPUTexture* colorTexture)
{
	if (colorTexture == nullptr)
	{
		return;
	}

	m_renderBackend->BindBackBuffer();
	m_renderBackend->BindPostProcessInputs(colorTexture, m_defaultWhiteTexture, m_defaultNormalTexture);
	Shader* copyShader =
		m_postProcessCopyShaderResource.IsValid() ? m_postProcessCopyShaderResource->GetShader() : nullptr;
	m_renderBackend->DrawFullscreenTriangle(copyShader, L"FinalCopyToBackBuffer");
	m_renderBackend->UnbindAllShaderResourceViews();
}

void Renderer::PrepareConstants(ViewportInfo const& viewport)
{
	// Prepare light constants
	LightConstants lightConstants  = LightConstants();
	int            pointLightCount = 0;
	int            spotLightCount  = 0;
	for (LightInfo const& light : viewport.m_lights)
	{
		Vec3 gpuColor;
		gpuColor.x = light.m_color.r / 255.f;
		gpuColor.y = light.m_color.g / 255.f;
		gpuColor.z = light.m_color.b / 255.f;

		switch (light.m_type)
		{
		case LightType::Directional:
		{
			lightConstants.m_directionalLight.m_direction = light.m_transform.GetIBasis3D().GetNormalized();
			lightConstants.m_directionalLight.m_intensity = light.m_intensity;
			lightConstants.m_directionalLight.m_color     = gpuColor;
			break;
		}
		case LightType::Omni:
		{
			if (pointLightCount > kMaxPointLights)
			{
				return;
			}
			lightConstants.m_pointLights[pointLightCount].m_position    = light.m_transform.GetTranslation3D();
			lightConstants.m_pointLights[pointLightCount].m_intensity   = light.m_intensity;
			lightConstants.m_pointLights[pointLightCount].m_color       = gpuColor;
			lightConstants.m_pointLights[pointLightCount].m_range       = light.m_range;
			lightConstants.m_pointLights[pointLightCount].m_attenuation = light.m_attenuation;
			++pointLightCount;

			break;
		}
		case LightType::Spot:
		{
			if (spotLightCount > kMaxSpotLights)
			{
				return;
			}
			lightConstants.m_spotLights[spotLightCount].m_position    = light.m_transform.GetTranslation3D();
			lightConstants.m_spotLights[spotLightCount].m_intensity   = light.m_intensity;
			lightConstants.m_spotLights[spotLightCount].m_color       = gpuColor;
			lightConstants.m_spotLights[spotLightCount].m_range       = light.m_range;
			lightConstants.m_spotLights[spotLightCount].m_attenuation = light.m_attenuation;
			lightConstants.m_spotLights[spotLightCount].m_direction   = light.m_transform.GetIBasis3D().GetNormalized();
			lightConstants.m_spotLights[spotLightCount].m_spotAngle   = light.m_spotAngle;
			lightConstants.m_spotLights[spotLightCount].m_spotAttenuation = light.m_spotAttenuation;
			++spotLightCount;

			break;
		}
		}
	}
	lightConstants.m_pointLightCount = pointLightCount;
	lightConstants.m_spotLightCount  = spotLightCount;
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

	// 1) Scene opaque objects write depth first
	for (RenderRequest const& request : viewport.m_renderRequests[(int)RenderRequestPass::Opaque])
	{
		ExecuteRenderRequest(request);
	}

	// 2) Gizmos world objects
	//    X-Ray objects use DepthMode::DISABLED / future GREATER to draw behind geometry.
	//    Normal objects use DepthMode::READ_WRITE_LESS_EQUAL to draw in front.
	for (RenderRequest const& request : DebugGizmos::GetRenderRequests(RenderRequestPass::Opaque))
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
	Shader* copyShader =
		m_postProcessCopyShaderResource.IsValid() ? m_postProcessCopyShaderResource->GetShader() : nullptr;
	m_renderBackend->DrawFullscreenTriangle(copyShader, L"CopyPostProcessToOutput");
	m_renderBackend->UnbindAllShaderResourceViews();
}

void Renderer::RenderUI(ViewportInfo const& viewport)
{
	CameraContext uiCameraData = CameraContext();
	uiCameraData.SetOrthogonal(Vec2::Zero, (Vec2)viewport.m_outputResolution, 0.f, 1.f);
	m_renderBackend->BindCamera(uiCameraData);

	m_renderBackend->BindRenderTarget(viewport.m_viewportOutputTexture);

	// 3) Gizmos screen text / messages
	for (RenderRequest const& request : DebugGizmos::GetRenderRequests(RenderRequestPass::UI))
	{
		ExecuteRenderRequest(request);
	}
	m_renderBackend->SetBlendMode(BlendMode::ALPHA);
	m_renderBackend->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
	m_renderBackend->SetDepthMode(DepthMode::READ_ONLY_ALWAYS);

	std::vector<RenderRequest> const& uiRequests = viewport.m_renderRequests[(int)RenderRequestPass::UI];
	for (size_t requestIndex = 0; requestIndex < uiRequests.size(); ++requestIndex)
	{
		ExecuteRenderRequest(uiRequests[requestIndex]);
	}
}

Shader* Renderer::CreateShader(
	std::string const& shaderName, std::string const& shaderSource, std::string const& shaderSourcePath)
{
	return m_renderBackend->CreateShader(shaderName, shaderSource, shaderSourcePath);
}

Ref<ShaderResource> Renderer::GetBuiltinShaderResource(
	std::string const& shaderName, std::string_view shaderSource)
{
	auto const found = m_builtinShaderResources.find(shaderName);
	if (found != m_builtinShaderResources.end())
	{
		return found->second;
	}

	Ref<ShaderResource> shaderResource = CreateRef<ShaderResource>();
	shaderResource->SetName(shaderName);
	shaderResource->SetShader(CreateShader(shaderName, std::string(shaderSource)));
	m_builtinShaderResources.emplace(shaderName, shaderResource);
	return shaderResource;
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
