//-----------------------------------------------------------------------------------------------
// SimpleTriangleFont.hpp
//	- Provided by Squirrel Eiserloh for use until we add Texture/SpriteSheet/BitmapFonts later!
//	- Create vertex arrays of triangles that spell out simple ASCII text
//	- Can further transform/distort font triangles using any math you see fit!
//
// A simple utility file for creating basic 5x9 pixel fonts out of pure triangles, i.e. does not
// require any external bitmap or TrueType fonts (or textures, or anything at all) in Data.
//-----------------------------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>
struct Vertex;
struct Rgba8;
struct Vec2;


//------------------------------------------------------------------------------------------------
void AddVertsForTextTriangles2D( std::vector<Vertex>& verts, std::string const& text, Vec2 const& startMins, float cellHeight, const Rgba8& color, float cellAspect = 0.56f, bool isFlipped=false, float spacingFraction = 0.2f );
float GetSimpleTriangleStringWidth( const std::string& text, float cellHeight, float cellAspect = 0.56f, float spacingFraction = 0.2f );

