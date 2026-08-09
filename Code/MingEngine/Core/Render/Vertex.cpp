#include "MingEngine/Core/Render/Vertex.hpp"

Vertex::Vertex()
	: m_position(0.f, 0.f, 0.f), m_color(Color()), m_uv(0.f, 0.f), m_tangent(Vector3::Zero), m_bitangent(Vector3::Zero),
	  m_normal(Vector3::Zero)
{
}

Vertex::Vertex(Vector3 const& position)
	: m_position(position), m_color(Color()), m_uv(0.f, 0.f), m_tangent(Vector3::Zero), m_bitangent(Vector3::Zero),
	  m_normal(Vector3::Zero)
{
}

Vertex::Vertex(Vector3 const& position, Color const& color)
	: m_position(position), m_color(color), m_uv(0.f, 0.f), m_tangent(Vector3::Zero), m_bitangent(Vector3::Zero),
	  m_normal(Vector3::Zero)
{
}

Vertex::Vertex(Vector3 const& position, Color const& color, Vector2 const& uvTexCoords)
	: m_position(position), m_color(color), m_uv(uvTexCoords), m_tangent(Vector3::Zero), m_bitangent(Vector3::Zero),
	  m_normal(Vector3::Zero)
{
}

Vertex::Vertex(
	Vector3 const& position,
	Color const&   color,
	Vector2 const& uvTexCoords,
	Vector3 const& tangent,
	Vector3 const& bitangent,
	Vector3 const& normal)
	: m_position(position), m_color(color), m_uv(uvTexCoords), m_tangent(tangent), m_bitangent(bitangent),
	  m_normal(normal)
{
}

Vertex::Vertex(
	float         px,
	float         py,
	float         pz,
	unsigned char r,
	unsigned char g,
	unsigned char b,
	unsigned char a,
	float         u,
	float         v,
	float         tx,
	float         ty,
	float         tz,
	float         bx,
	float         by,
	float         bz,
	float         nx,
	float         ny,
	float         nz)
	: m_position(px, py, pz), m_color(r, g, b, a), m_uv(u, v), m_tangent(tx, ty, tz), m_bitangent(bx, by, bz),
	  m_normal(nx, ny, nz)
{
}
