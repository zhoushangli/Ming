#pragma once

// DebugGizmos — static debug drawing system
//
// Responsibilities:
// 1) Owns DebugObject instances and manages their lifetimes (creation, timeout expiry).
// 2) Provides per-frame render request queues to Renderer.
// 3) Does not render directly; Renderer executes all RenderRequests uniformly.
//
// Render flow (inside Renderer):
// - PrepareRenderRequests builds all pass queues once per viewport.
// - RenderOpaque and RenderUI execute their corresponding queues.
//
// Lifecycle (driven by Renderer):
//   Startup  → Shutdown
//   BeginFrame → [Prepare* per viewport] → EndFrame
//
// TODO: consider merging with EditorGizmos later.

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/CylinderZ3.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"

class CameraContext;
class Renderer;

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

class DebugGizmos
{
public:
	// — Lifecycle —
	static void Startup(DebugRenderConfig const& config);
	static void Shutdown();

	// — Per-frame —
	// Clear render request queues from the previous frame
	static void BeginFrame();
	// Update DebugObject lifetimes and remove expired objects
	static void EndFrame();

	// — Control —
	static void SetVisible();
	static void SetHidden();
	static void Clear();

	// — Prepare —
	static void PrepareRenderRequests();

	// — Render request access —
	static std::vector<RenderRequest> const& GetRenderRequests(RenderRequestPass pass);

	// — Geometry (world space) —
	static void AddWorldSphere(
		Vec3 const&     center,
		float           radius,
		float           duration,
		Color const&    startColor = Color::White,
		Color const&    endColor   = Color::White,
		DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldWireSphere(
		Vec3 const&     center,
		float           radius,
		float           duration,
		Color const&    startColor = Color::White,
		Color const&    endColor   = Color::White,
		DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldCylinder(
		Vec3 const&     start,
		Vec3 const&     end,
		float           radius,
		float           duration,
		Color const&    startColor = Color::White,
		Color const&    endColor   = Color::White,
		DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldWireCylinder(
		Vec3 const&     start,
		Vec3 const&     end,
		float           radius,
		float           duration,
		Color const&    startColor = Color::White,
		Color const&    endColor   = Color::White,
		DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldWireCylinder(
		CylinderZ3 const& cylinder,
		Color const&      color,
		float             duration,
		DebugRenderMode   mode = DebugRenderMode::USE_DEPTH);

	static void AddWorldAABB(
		AABB3 const&    bounds,
		float           duration,
		Color const&    startColor = Color::White,
		Color const&    endColor   = Color::White,
		DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldAABB(
		AABB3 const&     bounds,
		Matrix4x4 const& transform,
		float            duration,
		Color const&     startColor = Color::White,
		Color const&     endColor   = Color::White,
		DebugRenderMode  mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldWireAABB(
		AABB3 const&    bounds,
		float           duration,
		Color const&    startColor = Color::White,
		Color const&    endColor   = Color::White,
		DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldWireAABB(
		AABB3 const&     bounds,
		Matrix4x4 const& transform,
		float            duration,
		Color const&     startColor = Color::White,
		Color const&     endColor   = Color::White,
		DebugRenderMode  mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldCapsule(
		Vec3 const&     start,
		Vec3 const&     end,
		float           radius,
		float           duration,
		Color const&    startColor = Color::White,
		Color const&    endColor   = Color::White,
		DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldWireCapsule(
		Vec3 const&     start,
		Vec3 const&     end,
		float           radius,
		float           duration,
		Color const&    startColor = Color::White,
		Color const&    endColor   = Color::White,
		DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldArrow(
		Vec3 const&     start,
		Vec3 const&     end,
		float           radius,
		float           duration,
		Color const&    startColor = Color::White,
		Color const&    endColor   = Color::White,
		DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldWireArrow(
		Vec3 const&     start,
		Vec3 const&     end,
		float           radius,
		float           duration,
		Color const&    startColor = Color::White,
		Color const&    endColor   = Color::White,
		DebugRenderMode mode       = DebugRenderMode::USE_DEPTH);

	static void AddBasis(
		Matrix4x4 const& transform,
		float            duration,
		float            length,
		float            radius,
		float            colorScale = 1.0f,
		float            alphaScale = 1.0f,
		DebugRenderMode  mode       = DebugRenderMode::USE_DEPTH);

	static void AddWorldBasis(
		Matrix4x4 const& transform, float duration, DebugRenderMode mode = DebugRenderMode::USE_DEPTH);

	static void AddWorldText(
		std::string const& text,
		Matrix4x4 const&   transform,
		float              textHeight,
		Vec2 const&        alignment,
		float              duration,
		Color const&       startColor = Color::White,
		Color const&       endColor   = Color::White,
		DebugRenderMode    mode       = DebugRenderMode::USE_DEPTH);

	// — Geometry (screen space) —
	static void AddScreenText(
		std::string const& text,
		AABB2 const&       box,
		float              cellHeight,
		Vec2 const&        alignment,
		float              duration,
		Color const&       startColor = Color::White,
		Color const&       endColor   = Color::White);

	static void AddMessage(
		std::string const& text,
		float              duration   = 0.f,
		Color const&       startColor = Color::White,
		Color const&       endColor   = Color::White);

	static void AddWorldGrid(float duration = -1.f, int halfExtent = 50);

private:
	// Console command handlers
	static bool Command_DebugRenderClear(EventArgs& args);
	static bool Command_DebugRenderToggle(EventArgs& args);
};
