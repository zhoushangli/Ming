#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Rgba8.hpp"

struct Vertex_PCU
{
public:
	Vertex_PCU();
	Vertex_PCU(Vec3 const& position);
	Vertex_PCU(Vec3 const& position, Rgba8 const& color);
	Vertex_PCU(Vec3 const& position, Rgba8 const& color, Vec2 const& uvTexCoords);

	Vertex_PCU& operator=(const Vertex_PCU& other)
	{
		if (this != &other) {
			m_position = other.m_position;
			m_color = other.m_color;
			m_uvTexCoords = other.m_uvTexCoords;
		}
		return *this;
	}

public:
	Vec3 m_position;
	Rgba8 m_color;
	Vec2 m_uvTexCoords;
};