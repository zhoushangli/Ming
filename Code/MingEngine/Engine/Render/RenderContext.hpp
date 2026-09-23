#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Render/Color.hpp"
#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Engine/Render/PostProcessChain.hpp"
#include "MingEngine/Engine/Render/RenderTypes.hpp"
#include "MingEngine/Engine/Render/TextureBindingSlots.hpp"

#include <array>
#include <vector>

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

struct LightData
{
	LightType m_type = LightType::Omni;

	Color m_color           = Color::White;
	float m_intensity       = 1.f;
	float m_range           = 1.f;
	float m_attenuation     = 1.f;
	float m_spotAngle       = 45.f;
	float m_spotAttenuation = 1.f;
};

enum class InstanceBaseType
{
	None,
	Mesh,
	Light,
};

// Registry entry for one registered mesh, addressed by a Mesh RID.
// e.g. MeshRegisterResource() creates one entry and MeshFree() removes it
// The resource itself is intentionally not stored here, so meshes can unload freely.
struct MeshData
{
};

// One scenario owns the instances that are drawn together.
// e.g. a Viewport holds a Scenario RID and Render() iterates m_instances of that Scenario
struct ScenarioData
{
	std::vector<RID> m_instances;
};

// One render instance: the Base RID decides how the instance is drawn.
// e.g. InstanceSetBase(instance, meshRID) makes it an InstanceBaseType::Mesh instance
struct Instance
{
	RID m_base     = RID::Invalid;
	RID m_scenario = RID::Invalid;

	InstanceBaseType m_baseType = InstanceBaseType::None;

	Matrix4x4 m_transform = Matrix4x4::Identity;
	Color     m_tint      = Color::White;
	bool      m_visible   = true;
};

// How a camera projects onto the clip space it renders with.
// e.g. CameraSetPerspective() writes Perspective and CameraSetOrthographic() writes Orthographic
enum class CameraMode
{
	Orthographic,
	Perspective,
};

// Minimal camera state addressed by one Camera RID on the RenderServer.
// e.g. CameraSet*() writes these fields and Projection turns them into matrices
struct CameraData
{
	CameraMode m_mode = CameraMode::Perspective;

	Matrix4x4 m_cameraToWorld = Matrix4x4::Identity;

	float m_nearZ = 0.1f;
	float m_farZ  = 100.f;

	// Vertical FOV in degrees, meaningful for Perspective cameras only.
	float m_fovDegrees = 60.f;

	// Vertical extent in world units, meaningful for Orthographic cameras only.
	float m_size = 1.f;
};

// All per-viewport render state, addressed by one Viewport RID on the RenderServer.
// e.g. Renderer::RenderViewport() reads the camera, targets, and request buckets from here
struct ViewportData
{
	// 1) Game fills cameras, dimensions, requests, lights, and post-process passes.
	// 2) Renderer creates and resizes the GPU textures below.
	// 3) All transient data and GPU resources belong to this Viewport only.
	bool m_active = false;

	// Camera that renders this viewport; RID::Invalid means UI only.
	// e.g. Camera3D binds itself here on EnterTree via ViewportSetCamera()
	RID m_camera   = RID::Invalid;
	RID m_scenario = RID::Invalid;

	// output resolution indicates the size of the render target
	// output rect indicates the portion of the render target to render to
	IntVec2 m_outputResolution = IntVec2::Zero;
	AABB2   m_outputRect       = AABB2::Unit;
	Color   m_clearColor       = Color(47, 54, 65, 255);

	GPUTexture*      m_viewportOutputTexture = nullptr;
	GPUTexture*      m_sceneColorTexture     = nullptr;
	GPUTexture*      m_sceneDepthTexture     = nullptr;
	GPUTexture*      m_sceneNormalTexture    = nullptr;
	GPUTexture*      m_pingTexture           = nullptr;
	GPUTexture*      m_pongTexture           = nullptr;
	PostProcessChain m_postProcessChain;

	std::array<std::vector<RenderRequest>, static_cast<size_t>(RenderRequestPass::Count)> m_renderRequests;

	// Will be filled in pre render stage
	// Collect lights from the scene
	// That's why we use pointers instead of RIDs, to represent their temporary exstence
	std::vector<Instance*> m_lights;
};
