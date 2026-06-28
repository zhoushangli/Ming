#include "MingEngine/Scene/SceneCommon.hpp"

#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

#include <array>

// formatter off

Matrix4x4 const OBJToEngineTransform =
	Matrix4x4(1.f, 0.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f);

Matrix4x4 const CameraToRenderTransform_Perspective =
	Matrix4x4(0.f, -1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f);

// formatter on

const Rgba8 DEBUG_PHYSICS_RADIUS_COLOR  = Rgba8(0, 255, 255);
const Rgba8 DEBUG_COSMETIC_RADIUS_COLOR = Rgba8(255, 0, 255);
const Rgba8 DEBUG_HEADING_COLOR         = Rgba8(255, 0, 0);
const Rgba8 DEBUG_LEFT_COLOR            = Rgba8(0, 255, 0);
const Rgba8 DEBUG_VELOCITY_COLOR        = Rgba8(255, 255, 0);

namespace
{
constexpr int kDebugLineVertexCount = 6;
constexpr int kDebugLineBufferCount = 256;

VertexBuffer* GetNextDebugLineBuffer()
{
	static std::array<VertexBuffer*, kDebugLineBufferCount> s_debugLineBuffers = {};
	static int                                              s_nextBufferIndex  = 0;

	Renderer* renderer = g_engine != nullptr ? g_engine->m_renderer : nullptr;
	if (renderer == nullptr)
	{
		return nullptr;
	}

	VertexBuffer*& buffer = s_debugLineBuffers[s_nextBufferIndex];
	s_nextBufferIndex     = (s_nextBufferIndex + 1) % kDebugLineBufferCount;

	if (buffer == nullptr)
	{
		buffer = renderer->CreateVertexBuffer(kDebugLineVertexCount * sizeof(Vertex), sizeof(Vertex));
	}

	return buffer;
}
} // namespace

void DebugDrawLine(Vec2 const& start, Vec2 const& end, Rgba8 const& color, float width)
{
	// Calculate direction and perpendicular
	Vec2 dir           = end - start;
	Vec2 dirNormalized = dir.GetNormalized();
	Vec2 perp          = dirNormalized.GetRotatedBy90Degrees();

	float halfWidth = width * 0.5f;

	// Calculate the four corners of the line quad
	Vec2 v0 = start + perp * halfWidth;
	Vec2 v1 = start - perp * halfWidth;
	Vec2 v2 = end - perp * halfWidth;
	Vec2 v3 = end + perp * halfWidth;

	// Create two triangles (v0, v1, v2) and (v0, v2, v3)
	Vertex verts[6];
	verts[0] = Vertex(v0, color);
	verts[1] = Vertex(v1, color);
	verts[2] = Vertex(v2, color);

	verts[3] = Vertex(v0, color);
	verts[4] = Vertex(v2, color);
	verts[5] = Vertex(v3, color);

	Renderer* renderer = g_engine != nullptr ? g_engine->m_renderer : nullptr;
	if (renderer == nullptr)
	{
		return;
	}

	VertexBuffer* vertexBuffer = GetNextDebugLineBuffer();
	if (vertexBuffer == nullptr)
	{
		return;
	}

	renderer->CopyCPUToGPU(verts, sizeof(verts), vertexBuffer);
}

void DebugDrawCircle(Vec2 const& center, float radius, Rgba8 const& color, float width /*= 0.1f*/)
{
	constexpr int NUM_SEGMENTS = 32;
	float         angleStep    = 360.0f / NUM_SEGMENTS;

	for (int i = 0; i < NUM_SEGMENTS; ++i)
	{
		float startAngle = i * angleStep;
		float endAngle   = (i + 1) * angleStep;

		Vec2 start = center + Vec2::MakeFromPolarDegrees(startAngle, radius);
		Vec2 end   = center + Vec2::MakeFromPolarDegrees(endAngle, radius);

		DebugDrawLine(start, end, color, width);
	}
}
