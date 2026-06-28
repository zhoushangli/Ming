#include "MingEngine/Scene/3D/Skybox3D.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/IndexBuffer.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

namespace
{
float const kHalfSize = 500.f;
} // namespace

Skybox3D::Skybox3D(std::string const& imagePath) : VisualizeInstance3D(), m_imagePath(imagePath)
{
	m_texture = g_engine->m_renderer->CreateOrGetTexture(m_imagePath.c_str());

	std::vector<Vertex>       verts;
	std::vector<unsigned int> indexes;

	verts.reserve(24);
	indexes.reserve(36);

	auto AddFace = [&verts, &indexes](
					   Vec3 const& bl,
					   Vec3 const& br,
					   Vec3 const& tr,
					   Vec3 const& tl,
					   Vec2 const& uvBL,
					   Vec2 const& uvBR,
					   Vec2 const& uvTR,
					   Vec2 const& uvTL)
	{
		unsigned int const startIndex = static_cast<unsigned int>(verts.size());

		verts.emplace_back(bl, Rgba8::White, uvBL);
		verts.emplace_back(br, Rgba8::White, uvBR);
		verts.emplace_back(tr, Rgba8::White, uvTR);
		verts.emplace_back(tl, Rgba8::White, uvTL);

		// Front-facing from inside the cube.
		indexes.push_back(startIndex + 0);
		indexes.push_back(startIndex + 1);
		indexes.push_back(startIndex + 2);

		indexes.push_back(startIndex + 0);
		indexes.push_back(startIndex + 2);
		indexes.push_back(startIndex + 3);
	};

	// Atlas layout:
	//
	//        +Y
	// -X  -Z  +X  +Z
	//        -Y
	//
	// cellX: 0  1  2  3
	// cellY: 2  1  0, because image loading flips textures to bottom-left UV origin.
	//
	// Classic cross:
	//        [1,2] = +Y
	// [0,1] = -X, [1,1] = -Z, [2,1] = +X, [3,1] = +Z
	//        [1,0] = -Y

	float u0 = 0.f / 4.f;
	float u1 = 1.f / 4.f;
	float u2 = 2.f / 4.f;
	float u3 = 3.f / 4.f;
	float u4 = 4.f / 4.f;
	float v0 = 0.f / 3.f;
	float v1 = 1.f / 3.f;
	float v2 = 2.f / 3.f;
	float v3 = 3.f / 3.f;

	// clang-format off
	// +X
	AddFace(
		Vec3( kHalfSize,  kHalfSize, -kHalfSize),
		Vec3( kHalfSize, -kHalfSize, -kHalfSize),
		Vec3( kHalfSize, -kHalfSize,  kHalfSize),
		Vec3( kHalfSize,  kHalfSize,  kHalfSize),
		Vec2(u2, v2),
		Vec2(u2, v1),
		Vec2(u3, v1),
		Vec2(u3, v2)
	);

	// -X
	AddFace(
		Vec3(-kHalfSize, -kHalfSize, -kHalfSize),
		Vec3(-kHalfSize,  kHalfSize, -kHalfSize),
		Vec3(-kHalfSize,  kHalfSize,  kHalfSize),
		Vec3(-kHalfSize, -kHalfSize,  kHalfSize),
		Vec2(u1, v1),
		Vec2(u1, v2),
		Vec2(u0, v2),
		Vec2(u0, v1)
	);

	// -Z
	AddFace(
		Vec3(-kHalfSize, -kHalfSize, -kHalfSize),
		Vec3( kHalfSize, -kHalfSize, -kHalfSize),
		Vec3( kHalfSize,  kHalfSize, -kHalfSize),
		Vec3(-kHalfSize,  kHalfSize, -kHalfSize),
		Vec2(u1, v1),
		Vec2(u2, v1),
		Vec2(u2, v2),
		Vec2(u1, v2)
	);

	// +Z
	AddFace(
		Vec3( kHalfSize, -kHalfSize,  kHalfSize),
		Vec3(-kHalfSize, -kHalfSize,  kHalfSize),
		Vec3(-kHalfSize,  kHalfSize,  kHalfSize),
		Vec3( kHalfSize,  kHalfSize,  kHalfSize),
		Vec2(u3, v1),
		Vec2(u4, v1),
		Vec2(u4, v2),
		Vec2(u3, v2)
	);

	// +Y
	AddFace(
		Vec3(-kHalfSize,  kHalfSize, -kHalfSize),
		Vec3( kHalfSize,  kHalfSize, -kHalfSize),
		Vec3( kHalfSize,  kHalfSize,  kHalfSize),
		Vec3(-kHalfSize,  kHalfSize,  kHalfSize),
		Vec2(u1, v2),
		Vec2(u2, v2),
		Vec2(u2, v3),
		Vec2(u1, v3)
	);

	// -Y
	AddFace(
		Vec3(-kHalfSize, -kHalfSize,  kHalfSize),
		Vec3( kHalfSize, -kHalfSize,  kHalfSize),
		Vec3( kHalfSize, -kHalfSize, -kHalfSize),
		Vec3(-kHalfSize, -kHalfSize, -kHalfSize),
		Vec2(u1, v0),
		Vec2(u2, v0),
		Vec2(u2, v1),
		Vec2(u1, v1)
	);

	// clang-format on

	m_vertexBuffer = g_engine->m_renderer->CreateVertexBuffer(verts);
	m_indexBuffer  = g_engine->m_renderer->CreateIndexBuffer(indexes);
}

Skybox3D::~Skybox3D()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;
	delete m_indexBuffer;
	m_indexBuffer = nullptr;
}

RenderRequest Skybox3D::SubmitRenderRequest() const
{
	RenderRequest request;
	if (m_vertexBuffer == nullptr || m_indexBuffer == nullptr)
	{
		return request;
	}

	request.m_pass                                  = RenderRequestPass::Skybox;
	request.m_modelToWorld                          = GetWorldTransform();
	request.m_tint                                  = Rgba8::White;
	request.m_vertexBuffer                          = m_vertexBuffer;
	request.m_indexBuffer                           = m_indexBuffer;
	request.m_textures[SurfaceTextureSlot::Diffuse] = m_texture;
	request.m_shader                                = nullptr;
	request.m_blendMode                             = BlendMode::OPAQUE;
	request.m_depthMode                             = DepthMode::READ_ONLY_LESS_EQUAL;
	request.m_rasterizerMode                        = RasterizerMode::SOLID_CULL_BACK;
	request.m_samplerMode                           = SamplerMode::POINT_CLAMP;
	return request;
}
