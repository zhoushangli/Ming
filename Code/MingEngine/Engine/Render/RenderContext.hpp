#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Render/Color.hpp"
#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Engine/Render/PostProcessChain.hpp"
#include "MingEngine/Engine/Render/RenderTypes.hpp"
#include "MingEngine/Engine/Render/TextureBindingSlots.hpp"
#include "MingEngine/Scene/Resource/MaterialResource.hpp"

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

	// Material tint of this request; requests without a material keep the white default.
	// e.g. BuildInstanceRenderRequest() copies MaterialResource::m_tint here
	Color m_tint = Color::White;

	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer*  m_indexBuffer  = nullptr;

	// Surface textures indexed by SurfaceTextureSlot; a null entry makes ExecuteRenderRequest() bind
	// the Renderer default texture of that slot, e.g. white for Diffuse
	std::array<GPUTexture*, PostProcessTextureSlot::MaxSamplerSlots> m_textures = {};

	// Shader the request is drawn with; the backend rejects a null one when the request executes.
	// e.g. the RenderServer fills the default unlit shader when the material or caller picked none
	Shader* m_shader = nullptr;

	// Draw states resolved from the material; the defaults below match MaterialResource defaults.
	// e.g. a material with BlendMode::ALPHA arrives here as m_blendMode = BlendMode::ALPHA
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
// e.g. MeshCreate() creates one entry and MeshFree() removes it again
// The MeshResource itself is intentionally not stored here, so meshes can unload freely.
struct MeshData
{
	RID m_vertexBufferRID = RID::Invalid;
	RID m_indexBufferRID  = RID::Invalid;

	// Material this mesh draws with, kept alive by the registration.
	// e.g. MeshCreate() copies it from the MeshResource and the request build reads it live
	Ref<MaterialResource> m_materialResource;
};

// One registered GPU buffer the RenderServer owns, addressed by a VertexBuffer RID.
// e.g. MeshCreate() stores its upload here and MeshFree() releases the RID again
struct VertexBufferData
{
	VertexBuffer* m_vertexBuffer = nullptr;
};

// One registered GPU buffer the RenderServer owns, addressed by an IndexBuffer RID.
// e.g. MeshCreate() stores its upload here and MeshFree() releases the RID again
struct IndexBufferData
{
	IndexBuffer* m_indexBuffer = nullptr;
};

// One registered GPU texture the RenderServer owns, addressed by a Texture RID.
// e.g. TextureCreate() stores the upload here and TextureFree() destroys it through the Renderer
struct TextureData
{
	GPUTexture* m_texture = nullptr;
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
public:
	// 1) Game fills cameras, dimensions, requests, lights, and post-process passes.
	// 2) Renderer creates and resizes the GPU textures below.
	// 3) All transient data and GPU resources belong to this Viewport only.

	// m_presentToScreen indicates this viewport should present to the backbuffer
	// Which is like the main viewport
	bool m_active = false;
	bool m_presentToScreen = false;

	// Camera that renders this viewport; RID::Invalid means UI only.
	// e.g. Camera3D binds itself here on EnterTree via ViewportSetCamera()
	RID m_camera   = RID::Invalid;
	RID m_scenario = RID::Invalid;
	std::vector<RID> m_canvasLayers;

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

public:
	// Will be filled in pre render stage
	// Collect lights from the scene
	// That's why we use pointers instead of RIDs, to represent their temporary exstence
	using RenderRequestArray = std::array<std::vector<RenderRequest>, (int)RenderRequestPass::Count>;

	struct LightInstance
	{
		Instance*  m_instance  = nullptr;
		LightData* m_lightData = nullptr;
	};

	LightInstance              m_directionalLight;
	std::vector<LightInstance> m_pointLights;

	RenderRequestArray m_renderRequests;
};

struct CanvasItemData
{
	struct Command
	{
		enum Type
		{
			TYPE_RECT,
		};

		Type type;
		virtual ~Command() = default;
	};

	struct CommandRect : Command
	{
		AABB2 rect;
		Color color;

		CommandRect() { type = TYPE_RECT; }
	};

	CanvasItemData()                                 = default;
	CanvasItemData(CanvasItemData const&)            = delete;
	CanvasItemData& operator=(CanvasItemData const&) = delete;
	~CanvasItemData() { ClearCommands(); }

	void ClearCommands()
	{
		for (Command* command : m_commands)
		{
			delete command;
		}
		m_commands.clear();
	}

	RID              m_viewport = RID::Invalid;
	RID              m_parent   = RID::Invalid;
	RID              m_layer    = RID::Invalid;
	std::vector<RID> m_children;

	Vector2               m_position = Vector2::Zero;
	bool                  m_visible  = true;
	std::vector<Command*> m_commands;
	VertexBuffer*         m_vertexBuffer = nullptr;
};

struct CanvasLayerData
{
	CanvasLayerData() = default;
	~CanvasLayerData() = default;

	RID              m_viewport = RID::Invalid;
	std::vector<RID> m_children;
};
