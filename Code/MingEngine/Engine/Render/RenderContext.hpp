#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Engine/Render/D3D11RenderBackend.hpp"
#include "MingEngine/Engine/Render/PostProcessChain.hpp"

#include <array>
#include <vector>

class CameraContext;
class IndexBuffer;
class Shader;
class GPUTexture;
class VertexBuffer;

enum class RenderRequestPass
{
	Skybox,
	Opaque,
	Transparent,
	UI,
	Count
};

struct RenderRequest
{
	// One VisualizeInstance produces at most one request.
	// A missing vertex buffer represents an intentionally empty request.
	bool IsValid() const { return m_vertexBuffer != nullptr; }

	RenderRequestPass m_pass           = RenderRequestPass::Opaque;
	int               m_renderPriority = 0; // Lower numbers render first.

	Matrix4x4 m_modelToWorld = Matrix4x4::Identity;
	Color     m_tint         = Color::White;

	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer*  m_indexBuffer  = nullptr;

	std::array<GPUTexture*, PostProcessTextureSlot::MaxSamplerSlots> m_textures = {};

	Shader* m_shader = nullptr;

	BlendMode      m_blendMode      = BlendMode::OPAQUE;
	DepthMode      m_depthMode      = DepthMode::READ_WRITE_LESS_EQUAL;
	RasterizerMode m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	SamplerMode    m_samplerMode    = SamplerMode::POINT_CLAMP;
};

enum class LightType
{
	Omni,
	Directional,
	Spot,
};

struct LightInfo
{
	LightType m_type = LightType::Omni;

	Matrix4x4 m_transform       = Matrix4x4::Identity;
	Color     m_color           = Color::White;
	float     m_intensity       = 1.f;
	float     m_range           = 1.f;
	float     m_attenuation     = 1.f;
	float     m_spotAngle       = 45.f;
	float     m_spotAttenuation = 1.f;
};

class ViewportInfo
{
public:
	// 1) Game fills cameras, dimensions, requests, lights, and post-process passes.
	// 2) Renderer creates and resizes the GPU textures below.
	// 3) All transient data and GPU resources belong to this Viewport only.
	CameraContext* m_worldCamera;

	// output resolution indicates the size of the render target
	// output rect indicates the portion of the render target to render to
	IntVec2 m_outputResolution = IntVec2::Zero;
	AABB2   m_outputRect       = AABB2::Unit;
	Color   m_clearColor       = Color(47, 54, 65, 255);

	GPUTexture* m_viewportOutputTexture = nullptr;
	GPUTexture* m_sceneColorTexture     = nullptr;
	GPUTexture* m_sceneDepthTexture     = nullptr;
	GPUTexture* m_sceneNormalTexture    = nullptr;
	GPUTexture* m_pingTexture           = nullptr;
	GPUTexture* m_pongTexture           = nullptr;

	std::array<std::vector<RenderRequest>, static_cast<size_t>(RenderRequestPass::Count)> m_renderRequests;

	std::vector<LightInfo> m_lights;
	PostProcessChain       m_postProcessChain;
};
