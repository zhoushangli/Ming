#pragma once

#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Render/Color.hpp"
#include "MingEngine/Engine/Render/RenderTypes.hpp"
#include "MingEngine/Engine/Render/TextureBindingSlots.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include <array>

// How a mesh surface is drawn: shader, surface textures, tint and draw states.
// e.g. MeshResource holds one and BuildInstanceRenderRequest() reads it into a RenderRequest
class MaterialResource : public Resource
{
	MCLASS(MaterialResource, Resource)

public:
	MaterialResource()                                   = default;
	MaterialResource(MaterialResource const& copy)       = delete;
	MaterialResource& operator=(MaterialResource const&) = delete;

	bool CopyFrom(Resource&& other) override;

protected:
	static void BindMethods() {}

public:
	// Shader the material is drawn with; an invalid reference lets the Renderer bind its default shader.
	// e.g. the default material points at the builtin "DefaultUnlit" ShaderResource
	Ref<ShaderResource> m_shaderResource;

	// Surface textures indexed by SurfaceTextureSlot (0 Diffuse, 1 Normal, 2 SGE).
	// An invalid entry is drawn with the Renderer default texture of its slot, e.g. white for Diffuse
	std::array<Ref<TextureResource>, SurfaceTextureSlot::Count> m_textureResources;

	// Tint multiplied into the model color of every draw that uses this material.
	// e.g. Color::White keeps the textures and vertex colors unchanged
	Color m_tint = Color::White;

	BlendMode      m_blendMode      = BlendMode::OPAQUE;
	DepthMode      m_depthMode      = DepthMode::READ_WRITE_LESS_EQUAL;
	RasterizerMode m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	SamplerMode    m_samplerMode    = SamplerMode::POINT_CLAMP;
};
