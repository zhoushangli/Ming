#include "Engine/Core/Vertex.hpp"

Vertex::Vertex()
	: m_position(0.f, 0.f, 0.f)
	, m_color(Rgba8())
	, m_uvTexCoords(0.f, 0.f)
{
}

Vertex::Vertex(Vec3 const& position) 
	: m_position(position), m_color(Rgba8()), m_uvTexCoords(0.f, 0.f)
{
}

Vertex::Vertex(Vec3 const& position, Rgba8 const& color)
	: m_position(position), m_color(color), m_uvTexCoords(0.f, 0.f)
{
}

Vertex::Vertex(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords) 
	: m_position(position), m_color(color), m_uvTexCoords(uvTexCoords)
{
}

