#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"
#include "MingEngine/Core/Render/Color.hpp"

struct Vertex
{
public:
	Vertex();
	Vertex(Vector3 const& position);
	Vertex(Vector3 const& position, Color const& color);
	Vertex(Vector3 const& position, Color const& color, Vector2 const& uvTexCoords);
	Vertex(
		Vector3 const& position,
		Color const&   color,
		Vector2 const& uvTexCoords,
		Vector3 const& tangent,
		Vector3 const& bitangent,
		Vector3 const& normal);

	Vertex(
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
		float         nz);

public:
	Vector3 m_position;
	Color   m_color;
	Vector2 m_uv;
	Vector3 m_tangent;
	Vector3 m_bitangent;
	Vector3 m_normal;
};
