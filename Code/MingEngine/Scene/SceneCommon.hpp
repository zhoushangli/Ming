#pragma once

struct Matrix4x4;
struct Rgba8;
struct Vec2;

extern Matrix4x4 const CameraToRenderTransform_Perspective;
extern Matrix4x4 const OBJToEngineTransform;

void DebugDrawLine(Vec2 const& start, Vec2 const& end, Rgba8 const& color, float width = 0.1f);
void DebugDrawCircle(Vec2 const& center, float radius, Rgba8 const& color, float width = 0.1f);
