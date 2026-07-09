#pragma once

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/CylinderZ3.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"

class ViewportInfo;

enum class DebugRenderMode
{
	ALWAYS,
	USE_DEPTH,
	X_RAY,
};

struct DebugRenderConfig
{
	Renderer*   m_renderer = nullptr;
	std::string m_fontPath = "Data/Fonts/";
	std::string m_fontName = "SquirrelFixedFont";
};

// Setup
void DebugRenderSystemStartup(const DebugRenderConfig& config);
void DebugRenderSystemShutdown();

// Control
void DebugRenderSetVisible();
void DebugRenderSetHidden();
void DebugRenderClear();

// Output
void DebugRenderBeginFrame();
void DebugRenderWorld(const CameraContext& camera, ViewportInfo& viewport);
void DebugRenderScreen(const CameraContext& camera, ViewportInfo& viewport);
void DebugRenderEndFrame();

// Geometry
void DebugAddWorldSphere(
	const Vec3&     center,
	float           radius,
	float           duration,
	const Rgba8&    startColor = Rgba8::White,
	const Rgba8&    endColor   = Rgba8::White,
	DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldWireSphere(
	const Vec3&     center,
	float           radius,
	float           duration,
	const Rgba8&    startColor = Rgba8::White,
	const Rgba8&    endColor   = Rgba8::White,
	DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldCylinder(
	const Vec3&     start,
	const Vec3&     end,
	float           radius,
	float           duration,
	const Rgba8&    startColor = Rgba8::White,
	const Rgba8&    endColor   = Rgba8::White,
	DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldWireCylinder(
	const Vec3&     start,
	const Vec3&     end,
	float           radius,
	float           duration,
	const Rgba8&    startColor = Rgba8::White,
	const Rgba8&    endColor   = Rgba8::White,
	DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldWireCylinder(
	const CylinderZ3& cylinder, const Rgba8& color, float duration, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldAABB(
	const AABB3&    bounds,
	float           duration,
	const Rgba8&    startColor = Rgba8::White,
	const Rgba8&    endColor   = Rgba8::White,
	DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldAABB(
	const AABB3&     bounds,
	const Matrix4x4& transform,
	float            duration,
	const Rgba8&     startColor = Rgba8::White,
	const Rgba8&     endColor   = Rgba8::White,
	DebugRenderMode  mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldWireAABB(
	const AABB3&    bounds,
	float           duration,
	const Rgba8&    startColor = Rgba8::White,
	const Rgba8&    endColor   = Rgba8::White,
	DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldWireAABB(
	const AABB3&     bounds,
	const Matrix4x4& transform,
	float            duration,
	const Rgba8&     startColor = Rgba8::White,
	const Rgba8&     endColor   = Rgba8::White,
	DebugRenderMode  mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldCapsule(
	const Vec3&     start,
	const Vec3&     end,
	float           radius,
	float           duration,
	const Rgba8&    startColor = Rgba8::White,
	const Rgba8&    endColor   = Rgba8::White,
	DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldWireCapsule(
	const Vec3&     start,
	const Vec3&     end,
	float           radius,
	float           duration,
	const Rgba8&    startColor = Rgba8::White,
	const Rgba8&    endColor   = Rgba8::White,
	DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldArrow(
	const Vec3&     start,
	const Vec3&     end,
	float           radius,
	float           duration,
	const Rgba8&    startColor = Rgba8::White,
	const Rgba8&    endColor   = Rgba8::White,
	DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldWireArrow(
	const Vec3&     start,
	const Vec3&     end,
	float           radius,
	float           duration,
	const Rgba8&    startColor = Rgba8::White,
	const Rgba8&    endColor   = Rgba8::White,
	DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

void DebugAddBasis(
	const Matrix4x4& transform,
	float            duration,
	float            length,
	float            radius,
	float            colorScale = 1.0f,
	float            alphaScale = 1.0f,
	DebugRenderMode  mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldBasis(const Matrix4x4& transform, float duration, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

void DebugAddWorldText(
	const std::string& text,
	const Matrix4x4&   transform,
	float              textHeight,
	const Vec2&        alignment,
	float              duration,
	const Rgba8&       startColor = Rgba8::White,
	const Rgba8&       endColor   = Rgba8::White,
	DebugRenderMode    mode       = DebugRenderMode::USE_DEPTH);

void DebugAddWorldBillboardText(
	const std::string& text,
	const Vec3&        origin,
	float              textHeight,
	const Vec2&        alignment,
	float              duration,
	const Rgba8&       startColor = Rgba8::White,
	const Rgba8&       endColor   = Rgba8::White,
	DebugRenderMode    mode       = DebugRenderMode::USE_DEPTH);

void DebugAddScreenText(
	const std::string& text,
	const AABB2&       box,
	float              cellHeight,
	const Vec2&        alignment,
	float              duration,
	const Rgba8&       startColor = Rgba8::White,
	const Rgba8&       endColor   = Rgba8::White);

void DebugAddMessage(
	const std::string& text,
	float              duration   = 0.f,
	const Rgba8&       startColor = Rgba8::White,
	const Rgba8&       endColor   = Rgba8::White);

void DebugAddWorldGrid(float duration = -1.f, int halfExtent = 50);

// Console commands
bool Command_DebugRenderClear(EventArgs& args);
bool Command_DebugRenderToggle(EventArgs& args);
