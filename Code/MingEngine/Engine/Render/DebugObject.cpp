#include "MingEngine/Engine/Render/DebugObject.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Engine/Render/BitmapFont.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

using namespace Math;

DebugObject::~DebugObject()
{
	delete vertexBuffer;
	vertexBuffer = nullptr;
}

DebugObject::DebugObject(DebugObject&& other) noexcept { *this = std::move(other); }

DebugObject& DebugObject::operator=(DebugObject&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	delete vertexBuffer;
	type              = other.type;
	mode              = other.mode;
	totalDuration     = other.totalDuration;
	remainingDuration = other.remainingDuration;
	startColor        = other.startColor;
	endColor          = other.endColor;
	start             = other.start;
	end               = other.end;
	center            = other.center;
	radius            = other.radius;
	transform         = other.transform;
	text              = std::move(other.text);
	textHeight        = other.textHeight;
	alignment         = other.alignment;
	screenBox         = other.screenBox;
	verts             = std::move(other.verts);
	vertexBuffer      = other.vertexBuffer;
	m_texture         = other.m_texture;
	m_shaderResource  = std::move(other.m_shaderResource);

	other.vertexBuffer   = nullptr;
	other.m_texture      = nullptr;
	return *this;
}

void DebugObject::CreateVertexBuffer(Renderer& renderer)
{
	if (verts.empty())
	{
		return;
	}

	delete vertexBuffer;
	unsigned int const size = static_cast<unsigned int>(verts.size() * sizeof(Vertex));
	vertexBuffer            = renderer.CreateVertexBuffer(verts.data(), size, sizeof(Vertex));
}

DebugObject DebugObject::Clone(Renderer& renderer) const
{
	DebugObject copy;
	copy.type              = type;
	copy.mode              = mode;
	copy.totalDuration     = totalDuration;
	copy.remainingDuration = remainingDuration;
	copy.startColor        = startColor;
	copy.endColor          = endColor;
	copy.start             = start;
	copy.end               = end;
	copy.center            = center;
	copy.radius            = radius;
	copy.transform         = transform;
	copy.text              = text;
	copy.textHeight        = textHeight;
	copy.alignment         = alignment;
	copy.screenBox         = screenBox;
	copy.verts             = verts;
	copy.CreateVertexBuffer(renderer);
	return copy;
}

void DebugObject::UploadVertices(Renderer& renderer, std::vector<Vertex> const& vertices)
{
	if (vertices.empty())
	{
		return;
	}

	unsigned int const size = static_cast<unsigned int>(vertices.size() * sizeof(Vertex));
	if (vertexBuffer == nullptr)
	{
		vertexBuffer = renderer.CreateVertexBuffer(size, sizeof(Vertex));
	}
	else if (vertexBuffer->GetSize() < size)
	{
		vertexBuffer->Resize(size);
	}

	renderer.CopyCPUToGPU(vertices.data(), size, vertexBuffer);
}

bool DebugObject::UsesUniformColor() const
{
	switch (type)
	{
	case DebugObjectType::WORLD_SPHERE:
	case DebugObjectType::WORLD_WIRE_SPHERE:
	case DebugObjectType::WORLD_CYLINDER:
	case DebugObjectType::WORLD_WIRE_CYLINDER:
	case DebugObjectType::WORLD_CAPSULE:
	case DebugObjectType::WORLD_WIRE_CAPSULE:
	case DebugObjectType::WORLD_ARROW:
	case DebugObjectType::WORLD_WIRE_ARROW:
	case DebugObjectType::WORLD_AABB:
	case DebugObjectType::WORLD_WIRE_AABB:
		return true;
	default:
		return false;
	}
}

Rgba8 DebugObject::GetCurrentColor() const
{
	if (totalDuration <= 0.f)
	{
		return startColor;
	}

	float t = 1.f - remainingDuration / totalDuration;
	return Interpolate(startColor, endColor, GetClamped(t, 0.f, 1.f));
}

bool DebugObject::IsScreenObject() const
{
	return type == DebugObjectType::SCREEN_TEXT || type == DebugObjectType::MESSAGE;
}

void DebugObject::UpdateRenderData(Renderer& renderer, BitmapFont* font, int messageLine)
{
	m_texture = nullptr;
	m_shaderResource =
		ResourceLoader::Load(IsScreenObject() ? "res://Shaders/DefaultUI.hlsl" : "res://Shaders/DefaultUnlit.hlsl");

	if (UsesUniformColor())
	{
		std::vector<Vertex> coloredVerts = verts;
		for (Vertex& vertex : coloredVerts)
		{
			vertex.m_color = GetCurrentColor();
		}
		UploadVertices(renderer, coloredVerts);
		return;
	}

	if (type == DebugObjectType::WORLD_GRID)
	{
		return;
	}

	if (font == nullptr)
	{
		return;
	}

	std::vector<Vertex> textVerts;
	textVerts.reserve(1024);
	if (type == DebugObjectType::WORLD_TEXT)
	{
		font->AddVertsForText3DAtOriginXForward(textVerts, textHeight, text, GetCurrentColor(), 1.f, alignment);
		TransformVertexArray3D(textVerts, transform);
	}
	else
	{
		AABB2 box = screenBox;
		if (type == DebugObjectType::MESSAGE)
		{
			float const cellHeight = textHeight > 0.f ? textHeight : 20.f;
			float const top        = -(cellHeight + 2.f) * static_cast<float>(messageLine);
			box                    = AABB2(Vec2(10.f, top - cellHeight), Vec2(1000.f, top));
		}
		font->AddVertsForTextInBox2D(
			textVerts, text, box, textHeight > 0.f ? textHeight : 20.f, GetCurrentColor(), 1.f, alignment,
			TextBoxMode::SHRINK_TO_FIT);
	}

	m_texture = font->GetTexture();
	UploadVertices(renderer, textVerts);
}

RenderRequest DebugObject::SubmitRenderRequest() const
{
	RenderRequest request;
	request.m_pass           = IsScreenObject() ? RenderRequestPass::UI : RenderRequestPass::Opaque;
	request.m_modelToWorld   = Matrix4x4::Identity;
	request.m_tint           = Rgba8::White;
	request.m_vertexBuffer   = vertexBuffer;
	request.m_indexBuffer    = nullptr;
	request.m_shader         = m_shaderResource.IsValid() ? m_shaderResource->GetShader() : nullptr;
	request.m_blendMode      = BlendMode::ALPHA;
	request.m_depthMode      = IsScreenObject() ? DepthMode::READ_ONLY_ALWAYS
		: (mode == DebugRenderMode::USE_DEPTH ? DepthMode::READ_WRITE_LESS_EQUAL : DepthMode::DISABLED);
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
	switch (type)
	{
	case DebugObjectType::WORLD_WIRE_SPHERE:
	case DebugObjectType::WORLD_WIRE_CYLINDER:
	case DebugObjectType::WORLD_WIRE_CAPSULE:
	case DebugObjectType::WORLD_WIRE_ARROW:
	case DebugObjectType::WORLD_WIRE_AABB:
		request.m_rasterizerMode = RasterizerMode::WIREFRAME_CULL_NONE;
		break;
	default:
		break;
	}
	request.m_samplerMode = m_texture != nullptr ? SamplerMode::BILINEAR_CLAMP : SamplerMode::POINT_CLAMP;
	if (m_texture != nullptr)
	{
		request.m_textures[0] = m_texture;
	}
	return request;
}
