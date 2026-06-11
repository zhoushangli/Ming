#pragma once

#include "MingEngine/Engine/Math/AABB2.hpp"
#include "MingEngine/Engine/Math/IntVec2.hpp"
#include "MingEngine/Engine/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Render/D3D11RenderBackend.hpp"
#include "MingEngine/Engine/Render/PostProcessChain.hpp"
#include "MingEngine/Engine/Render/Rgba8.hpp"

#include <array>
#include <vector>

class CameraContext;
class IndexBuffer;
class Shader;
class Texture;
class VertexBuffer;

enum class RenderRequestPass
{
	Opaque,
	Skybox,
	Transparent,
	UI,
	Count
};

struct RenderRequest
{
	// One VisualizeInstance produces at most one request.
	// A missing vertex buffer represents an intentionally empty request.
	bool IsValid() const { return m_vertexBuffer != nullptr; }

	RenderRequestPass m_pass = RenderRequestPass::Opaque;

	Matrix4x4 m_modelToWorld = Matrix4x4::Identity;
	Rgba8 m_tint             = Rgba8::White;

	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer* m_indexBuffer   = nullptr;

	std::array<Texture*, PostProcessTextureSlot::MaxSamplerSlots> m_textures = {};

	Shader* m_shader = nullptr;

	BlendMode m_blendMode           = BlendMode::OPAQUE;
	DepthMode m_depthMode           = DepthMode::READ_WRITE_LESS_EQUAL;
	RasterizerMode m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	SamplerMode m_samplerMode       = SamplerMode::POINT_CLAMP;
};

enum class LightType
{
	POINT,
	DIRECTIONAL,
};

struct LightInfo
{
	LightType m_type = LightType::POINT;

	Rgba8 m_color     = Rgba8::White;
	Vec3 m_direction  = Vec3::Forward;
	float m_intensity = 0.f;
	Vec3 m_position   = Vec3::Zero;
	float m_range     = 0.f;
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
	AABB2 m_outputRect         = AABB2::Unit;
	Rgba8 m_clearColor         = Rgba8(47, 54, 65, 255);

	Texture* m_viewportOutputTexture = nullptr;
	Texture* m_sceneColorTexture     = nullptr;
	Texture* m_sceneDepthTexture     = nullptr;
	Texture* m_sceneNormalTexture    = nullptr;
	Texture* m_pingTexture           = nullptr;
	Texture* m_pongTexture           = nullptr;

	std::array<std::vector<RenderRequest>, static_cast<size_t>(RenderRequestPass::Count)> m_renderRequests;

	std::vector<LightInfo> m_lights;
	PostProcessChain m_postProcessChain;
};
