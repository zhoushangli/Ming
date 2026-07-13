#pragma once

#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"

class BitmapFont;
class GPUTexture;
class Renderer;
class VertexBuffer;

enum class DebugObjectType
{
	WORLD_SPHERE,
	WORLD_WIRE_SPHERE,
	WORLD_CYLINDER,
	WORLD_WIRE_CYLINDER,
	WORLD_CAPSULE,
	WORLD_WIRE_CAPSULE,
	WORLD_ARROW,
	WORLD_WIRE_ARROW,
	WORLD_AABB,
	WORLD_WIRE_AABB,
	WORLD_TEXT,
	WORLD_GRID,
	SCREEN_TEXT,
	MESSAGE
};

class DebugObject
{
public:
	DebugObject() = default;
	~DebugObject();

	DebugObject(DebugObject const&)            = delete;
	DebugObject& operator=(DebugObject const&) = delete;
	DebugObject(DebugObject&& other) noexcept;
	DebugObject& operator=(DebugObject&& other) noexcept;

	void          CreateVertexBuffer(Renderer& renderer);
	DebugObject   Clone(Renderer& renderer) const;
	void          UpdateRenderData(Renderer& renderer, BitmapFont* font, int messageLine = -1);
	RenderRequest SubmitRenderRequest() const;
	Rgba8         GetCurrentColor() const;
	bool          IsScreenObject() const;

	DebugObjectType type = DebugObjectType::WORLD_SPHERE;
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH;

	float totalDuration     = 0.f;
	float remainingDuration = 0.f;

	Rgba8 startColor = Rgba8::White;
	Rgba8 endColor   = Rgba8::White;

	Vec3  start  = Vec3::Zero;
	Vec3  end    = Vec3::Zero;
	Vec3  center = Vec3::Zero;
	float radius = 0.f;

	Matrix4x4 transform;

	std::string text;
	float       textHeight = 0.f;
	Vec2        alignment  = Vec2(0.5f, 0.5f);
	AABB2       screenBox;

	std::vector<Vertex> verts;
	VertexBuffer*       vertexBuffer = nullptr;

private:
	void UploadVertices(Renderer& renderer, std::vector<Vertex> const& vertices);
	bool UsesUniformColor() const;

	GPUTexture*         m_texture = nullptr;
	Ref<ShaderResource> m_shaderResource;
};
