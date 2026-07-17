#include "MingEngine/Engine/Render/DebugGizmos.hpp"
#include "MingEngine/Engine/Render/DebugObject.hpp"

#include "MingEngine/Core/Clock.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Core/Time.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/BitmapFont.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include "MingEngine/EngineService/EngineService.hpp"

using namespace Math;

namespace
{
// — DebugGizmos internal static state —
DebugRenderConfig s_debugRenderConfig;

std::vector<DebugObject> s_debugObjects;
std::vector<DebugObject> s_debugMessages;

bool s_isVisible = true;

std::array<std::vector<RenderRequest>, static_cast<size_t>(RenderRequestPass::Count)> s_renderRequests;

void AddDebugObject(DebugObject&& object)
{
	if (object.mode != DebugRenderMode::X_RAY)
	{
		s_debugObjects.emplace_back(std::move(object));
		return;
	}

	Renderer* renderer = s_debugRenderConfig.m_renderer;
	if (renderer == nullptr)
	{
		return;
	}

	DebugObject depthObject = object.Clone(*renderer);
	depthObject.mode        = DebugRenderMode::USE_DEPTH;

	object.mode         = DebugRenderMode::ALWAYS;
	object.startColor.a = static_cast<unsigned char>(static_cast<float>(object.startColor.a) * 0.35f);
	object.endColor.a   = static_cast<unsigned char>(static_cast<float>(object.endColor.a) * 0.35f);

	// 1) Draw the translucent object through geometry.
	// 2) Draw the normal depth-tested object on top.
	s_debugObjects.emplace_back(std::move(object));
	s_debugObjects.emplace_back(std::move(depthObject));
}

DebugObject MakeDebugObject(
	DebugObjectType type,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode = DebugRenderMode::USE_DEPTH)
{
	DebugObject object;
	object.type              = type;
	object.mode              = mode;
	object.totalDuration     = duration;
	object.remainingDuration = duration;
	object.startColor        = startColor;
	object.endColor          = endColor;
	return object;
}

void CreateVertexBufferForObject(DebugObject& object)
{
	Renderer* renderer = s_debugRenderConfig.m_renderer;
	if (renderer == nullptr || object.verts.empty())
	{
		return;
	}

	object.CreateVertexBuffer(*renderer);
}

DebugObject MakeWorldSphereObject(
	DebugObjectType type,
	Vec3 const&     center,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	DebugObject object = MakeDebugObject(type, duration, startColor, endColor, mode);
	object.center      = center;
	object.radius      = radius;
	AddVertsForSphere3D(object.verts, center, radius, startColor);
	CreateVertexBufferForObject(object);
	return object;
}

DebugObject MakeWorldCylinderObject(
	DebugObjectType type,
	Vec3 const&     start,
	Vec3 const&     end,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	DebugObject object = MakeDebugObject(type, duration, startColor, endColor, mode);
	object.start       = start;
	object.end         = end;
	object.radius      = radius;
	AddVertsForCylinder3D(object.verts, start, end, radius, startColor);
	CreateVertexBufferForObject(object);
	return object;
}

DebugObject MakeWorldCapsuleObject(
	DebugObjectType type,
	Vec3 const&     start,
	Vec3 const&     end,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	DebugObject object = MakeDebugObject(type, duration, startColor, endColor, mode);
	object.start       = start;
	object.end         = end;
	object.radius      = radius;
	AddVertsForCapsule3D(object.verts, start, end, radius, startColor);
	CreateVertexBufferForObject(object);
	return object;
}

DebugObject MakeWorldArrowObject(
	DebugObjectType type,
	Vec3 const&     start,
	Vec3 const&     end,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	DebugObject object = MakeDebugObject(type, duration, startColor, endColor, mode);
	object.start       = start;
	object.end         = end;
	object.radius      = radius;
	AddVertsForArrow3D(object.verts, start, end, radius, startColor);
	CreateVertexBufferForObject(object);
	return object;
}

DebugObject MakeWorldAABBObject(
	DebugObjectType  type,
	AABB3 const&     bounds,
	Matrix4x4 const& transform,
	float            duration,
	Color const&     startColor,
	Color const&     endColor,
	DebugRenderMode  mode)
{
	DebugObject object = MakeDebugObject(type, duration, startColor, endColor, mode);
	object.transform   = transform;
	AddVertsForAABB3D(object.verts, bounds, startColor);
	TransformVertexArray3D(object.verts, transform);
	CreateVertexBufferForObject(object);
	return object;
}

void UpdateDebugObjectLifetimes(std::vector<DebugObject>& objects, float deltaSeconds)
{
	for (int i = (int)objects.size() - 1; i >= 0; --i)
	{
		DebugObject& obj = objects[i];

		if (obj.totalDuration < 0.f)
		{
			continue;
		}

		obj.remainingDuration -= deltaSeconds;

		if (obj.remainingDuration <= 0.f)
		{
			objects.erase(objects.begin() + i);
		}
	}
}
} // namespace

// — Lifecycle —
void DebugGizmos::Startup(DebugRenderConfig const& config)
{
	s_debugRenderConfig = config;
	s_debugObjects.clear();
	s_debugMessages.clear();
	s_isVisible = true;

	g_engine->m_eventSystem->RegisterEvent("Dev_DebugRenderClear", Command_DebugRenderClear);
	g_engine->m_eventSystem->RegisterEvent("Dev_DebugRenderToggle", Command_DebugRenderToggle);
	if (g_engineService != nullptr && g_engineService->m_console != nullptr)
	{
		g_engineService->m_console->AddCommand("DebugRenderClear", Command_DebugRenderClear);
		g_engineService->m_console->AddCommand("DebugRenderToggle", Command_DebugRenderToggle);
	}
}

void DebugGizmos::Shutdown()
{
	g_engine->m_eventSystem->UnregisterEvent("Dev_DebugRenderClear", Command_DebugRenderClear);
	g_engine->m_eventSystem->UnregisterEvent("Dev_DebugRenderToggle", Command_DebugRenderToggle);

	s_debugRenderConfig.m_renderer = nullptr;
	s_debugObjects.clear();
	s_debugMessages.clear();
	s_isVisible = false;
}

// — Per-frame —
void DebugGizmos::BeginFrame() {}

void DebugGizmos::EndFrame()
{
	// Update DebugObject lifetimes and remove expired objects
	float dt = (float)Clock::GetSystemClock().GetDeltaSeconds();
	UpdateDebugObjectLifetimes(s_debugObjects, dt);
	UpdateDebugObjectLifetimes(s_debugMessages, dt);
}

// — Control —
void DebugGizmos::SetVisible() { s_isVisible = true; }

void DebugGizmos::SetHidden() { s_isVisible = false; }

void DebugGizmos::Clear()
{
	s_debugObjects.clear();
	s_debugMessages.clear();
}

// — Prepare —
void DebugGizmos::PrepareRenderRequests()
{
	for (auto& requests : s_renderRequests)
	{
		requests.clear();
	}

	if (!s_isVisible)
	{
		return;
	}

	Renderer* renderer = s_debugRenderConfig.m_renderer;
	if (renderer == nullptr)
	{
		return;
	}

	// TODO: Restore BitmapFont creation when Renderer exposes a factory method
	BitmapFont* font = nullptr;

	for (DebugObject& obj : s_debugObjects)
	{
		obj.UpdateRenderData(*renderer, font);
		RenderRequest request = obj.SubmitRenderRequest();
		if (request.m_vertexBuffer != nullptr)
		{
			s_renderRequests[static_cast<size_t>(request.m_pass)].push_back(request);
		}
	}

	for (int messageIndex = 0; messageIndex < (int)s_debugMessages.size(); ++messageIndex)
	{
		DebugObject& obj = s_debugMessages[messageIndex];
		obj.UpdateRenderData(*renderer, font, (int)s_debugMessages.size() - messageIndex - 1);
		RenderRequest request = obj.SubmitRenderRequest();
		if (request.m_vertexBuffer != nullptr)
		{
			s_renderRequests[static_cast<size_t>(request.m_pass)].push_back(request);
		}
	}
}

// — Render request access —
std::vector<RenderRequest> const& DebugGizmos::GetRenderRequests(RenderRequestPass pass)
{
	return s_renderRequests[static_cast<size_t>(pass)];
}

// — Geometry (world space) —
void DebugGizmos::AddWorldSphere(
	Vec3 const&     center,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	AddDebugObject(
		MakeWorldSphereObject(DebugObjectType::WORLD_SPHERE, center, radius, duration, startColor, endColor, mode));
}

void DebugGizmos::AddWorldWireSphere(
	Vec3 const&     center,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	AddDebugObject(MakeWorldSphereObject(
		DebugObjectType::WORLD_WIRE_SPHERE,
		center,
		radius,
		duration,
		startColor,
		endColor,
		mode));
}

void DebugGizmos::AddWorldCylinder(
	Vec3 const&     start,
	Vec3 const&     end,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	AddDebugObject(MakeWorldCylinderObject(
		DebugObjectType::WORLD_CYLINDER,
		start,
		end,
		radius,
		duration,
		startColor,
		endColor,
		mode));
}

void DebugGizmos::AddWorldWireCylinder(
	Vec3 const&     start,
	Vec3 const&     end,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	AddDebugObject(MakeWorldCylinderObject(
		DebugObjectType::WORLD_WIRE_CYLINDER,
		start,
		end,
		radius,
		duration,
		startColor,
		endColor,
		mode));
}

void DebugGizmos::AddWorldWireCylinder(
	CylinderZ3 const& cylinder, Color const& color, float duration, DebugRenderMode mode)
{
	AddDebugObject(MakeWorldCylinderObject(
		DebugObjectType::WORLD_WIRE_CYLINDER,
		Vec3(cylinder.m_centerXY.x, cylinder.m_centerXY.y, cylinder.m_minMaxZ.m_min),
		Vec3(cylinder.m_centerXY.x, cylinder.m_centerXY.y, cylinder.m_minMaxZ.m_max),
		cylinder.m_radius,
		duration,
		color,
		color,
		mode));
}

void DebugGizmos::AddWorldAABB(
	AABB3 const& bounds, float duration, Color const& startColor, Color const& endColor, DebugRenderMode mode)
{
	AddDebugObject(MakeWorldAABBObject(
		DebugObjectType::WORLD_AABB,
		bounds,
		Matrix4x4::Identity,
		duration,
		startColor,
		endColor,
		mode));
}

void DebugGizmos::AddWorldAABB(
	AABB3 const&     bounds,
	Matrix4x4 const& transform,
	float            duration,
	Color const&     startColor,
	Color const&     endColor,
	DebugRenderMode  mode)
{
	AddDebugObject(
		MakeWorldAABBObject(DebugObjectType::WORLD_AABB, bounds, transform, duration, startColor, endColor, mode));
}

void DebugGizmos::AddWorldWireAABB(
	AABB3 const& bounds, float duration, Color const& startColor, Color const& endColor, DebugRenderMode mode)
{
	AddDebugObject(MakeWorldAABBObject(
		DebugObjectType::WORLD_WIRE_AABB,
		bounds,
		Matrix4x4::Identity,
		duration,
		startColor,
		endColor,
		mode));
}

void DebugGizmos::AddWorldWireAABB(
	AABB3 const&     bounds,
	Matrix4x4 const& transform,
	float            duration,
	Color const&     startColor,
	Color const&     endColor,
	DebugRenderMode  mode)
{
	AddDebugObject(
		MakeWorldAABBObject(DebugObjectType::WORLD_WIRE_AABB, bounds, transform, duration, startColor, endColor, mode));
}

void DebugGizmos::AddWorldCapsule(
	Vec3 const&     start,
	Vec3 const&     end,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	AddDebugObject(MakeWorldCapsuleObject(
		DebugObjectType::WORLD_CAPSULE,
		start,
		end,
		radius,
		duration,
		startColor,
		endColor,
		mode));
}

void DebugGizmos::AddWorldWireCapsule(
	Vec3 const&     start,
	Vec3 const&     end,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	AddDebugObject(MakeWorldCapsuleObject(
		DebugObjectType::WORLD_WIRE_CAPSULE,
		start,
		end,
		radius,
		duration,
		startColor,
		endColor,
		mode));
}

void DebugGizmos::AddWorldArrow(
	Vec3 const&     start,
	Vec3 const&     end,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	AddDebugObject(
		MakeWorldArrowObject(DebugObjectType::WORLD_ARROW, start, end, radius, duration, startColor, endColor, mode));
}

void DebugGizmos::AddWorldWireArrow(
	Vec3 const&     start,
	Vec3 const&     end,
	float           radius,
	float           duration,
	Color const&    startColor,
	Color const&    endColor,
	DebugRenderMode mode)
{
	AddDebugObject(MakeWorldArrowObject(
		DebugObjectType::WORLD_WIRE_ARROW,
		start,
		end,
		radius,
		duration,
		startColor,
		endColor,
		mode));
}

void DebugGizmos::AddBasis(
	Matrix4x4 const& transform,
	float            duration,
	float            length,
	float            radius,
	float            colorScale,
	float            alphaScale,
	DebugRenderMode  mode)
{
	Color const kAxisXColor(255, 70, 105, 255);
	Color const kAxisYColor(155, 225, 20, 255);
	Color const kAxisZColor(55, 160, 255, 255);

	Vec3 origin = transform.GetTranslation3D();
	Vec3 xEnd   = transform.TransformPosition3D(Vec3(length, 0.f, 0.f));
	Vec3 yEnd   = transform.TransformPosition3D(Vec3(0.f, length, 0.f));
	Vec3 zEnd   = transform.TransformPosition3D(Vec3(0.f, 0.f, length));

	Color xColor = Interpolate(Color::Black, kAxisXColor, colorScale);
	Color yColor = Interpolate(Color::Black, kAxisYColor, colorScale);
	Color zColor = Interpolate(Color::Black, kAxisZColor, colorScale);

	xColor.a = (unsigned char)GetClamped((float)xColor.a * alphaScale, 0.f, 255.f);
	yColor.a = (unsigned char)GetClamped((float)yColor.a * alphaScale, 0.f, 255.f);
	zColor.a = (unsigned char)GetClamped((float)zColor.a * alphaScale, 0.f, 255.f);

	AddWorldArrow(origin, xEnd, radius, duration, xColor, xColor, mode);
	AddWorldArrow(origin, yEnd, radius, duration, yColor, yColor, mode);
	AddWorldArrow(origin, zEnd, radius, duration, zColor, zColor, mode);
}

void DebugGizmos::AddWorldBasis(Matrix4x4 const& transform, float duration, DebugRenderMode mode)
{
	AddBasis(transform, duration, 1.0f, 0.1f, 1.0f, 1.0f, mode);
}

void DebugGizmos::AddWorldText(
	std::string const& text,
	Matrix4x4 const&   transform,
	float              textHeight,
	Vec2 const&        alignment,
	float              duration,
	Color const&       startColor,
	Color const&       endColor,
	DebugRenderMode    mode)
{
	DebugObject object = MakeDebugObject(DebugObjectType::WORLD_TEXT, duration, startColor, endColor, mode);
	object.text        = text;
	object.transform   = transform;
	object.textHeight  = textHeight;
	object.alignment   = alignment;
	AddDebugObject(std::move(object));
}

// — Geometry (screen space) —
void DebugGizmos::AddScreenText(
	std::string const& text,
	AABB2 const&       box,
	float              cellHeight,
	Vec2 const&        alignment,
	float              duration,
	Color const&       startColor,
	Color const&       endColor)
{
	DebugObject object =
		MakeDebugObject(DebugObjectType::SCREEN_TEXT, duration, startColor, endColor, DebugRenderMode::ALWAYS);
	object.text       = text;
	object.screenBox  = box;
	object.textHeight = cellHeight;
	object.alignment  = alignment;
	AddDebugObject(std::move(object));
}

void DebugGizmos::AddMessage(std::string const& text, float duration, Color const& startColor, Color const& endColor)
{
	DebugObject object =
		MakeDebugObject(DebugObjectType::MESSAGE, duration, startColor, endColor, DebugRenderMode::ALWAYS);
	object.text       = text;
	object.textHeight = 24.f;
	object.alignment  = Vec2(0.f, 0.5f);
	s_debugMessages.push_back(std::move(object));
}

void DebugGizmos::AddWorldGrid(float duration, int halfExtent)
{
	Color const kAxisXColor(255, 70, 105, 255);
	Color const kAxisYColor(155, 225, 20, 255);

	// Configurable parameters for grid generation.
	const float kLineHeight = 0.01f;

	// Thickness: only Base + Axis (Major treated same as Base)
	const float kBaseThickness = 0.002f;
	const float kAxisThickness = 0.01f;

	// Brightness: only Base + Axis (Major treated same as Base)
	const float kBaseBrightness = 0.25f;
	const float kAxisBrightness = 1.f;

	// Fade settings (hard-coded)
	const float kFadeStart = 10.f;
	const float kFadeEnd   = 30.f;

	// Segment size (hard-coded): smaller => smoother fade, more verts
	const float kSegmentLength = 1.f;

	int const clampedHalfExtent = halfExtent < 0 ? 0 : halfExtent;

	DebugObject object =
		MakeDebugObject(DebugObjectType::WORLD_GRID, duration, Color::White, Color::White, DebugRenderMode::USE_DEPTH);
	object.verts.clear();
	object.verts.reserve((clampedHalfExtent * 2 + 1) * 2 * 36);

	auto ComputeAlphaForDistance = [&](float d) -> unsigned char
	{
		float a = RangeMapClamped(d, kFadeStart, kFadeEnd, 255.f, 0.f);
		return (unsigned char)GetClamped(a, 0.f, 255.f);
	};

	auto AddSegmentAABB = [&object, &ComputeAlphaForDistance](AABB3 const& aabb, Color baseColor)
	{
		// Use segment center distance to compute alpha
		Vec3  center = aabb.GetCenter();
		float dist   = sqrtf(center.x * center.x + center.y * center.y);
		baseColor.a  = ComputeAlphaForDistance(dist);
		AddVertsForAABB3D(object.verts, aabb, baseColor);
	};

	for (int lineIndex = -clampedHalfExtent; lineIndex <= clampedHalfExtent; ++lineIndex)
	{
		bool const isAxis = (lineIndex == 0);

		float lineThickness = kBaseThickness;
		float brightness    = kBaseBrightness;

		if (isAxis)
		{
			lineThickness = kAxisThickness;
			brightness    = kAxisBrightness;
		}

		float const lineOffset    = static_cast<float>(lineIndex);
		float const halfThickness = lineThickness * 0.5f;

		Color xParallelColor = kAxisXColor * brightness;
		Color yParallelColor = kAxisYColor * brightness;

		if (!isAxis)
		{
			xParallelColor = Color::Gray;
			yParallelColor = Color::Gray;
		}

		// Split each long strip into small segments along its length.
		float const minCoord = -static_cast<float>(clampedHalfExtent);
		float const maxCoord = static_cast<float>(clampedHalfExtent);

		// X-axis parallel lines (vary Y, span X)
		for (float x = minCoord; x < maxCoord; x += kSegmentLength)
		{
			float x0 = x;
			float x1 = x + kSegmentLength;
			if (x1 > maxCoord)
			{
				x1 = maxCoord;
			}

			AddSegmentAABB(
				AABB3(Vec3(x0, lineOffset - halfThickness, 0.f), Vec3(x1, lineOffset + halfThickness, kLineHeight)),
				xParallelColor);
		}

		// Y-axis parallel lines (vary X, span Y)
		for (float y = minCoord; y < maxCoord; y += kSegmentLength)
		{
			float y0 = y;
			float y1 = y + kSegmentLength;
			if (y1 > maxCoord)
			{
				y1 = maxCoord;
			}

			AddSegmentAABB(
				AABB3(Vec3(lineOffset - halfThickness, y0, 0.f), Vec3(lineOffset + halfThickness, y1, kLineHeight)),
				yParallelColor);
		}
	}

	CreateVertexBufferForObject(object);
	AddDebugObject(std::move(object));
}

// — Console commands —
bool DebugGizmos::Command_DebugRenderClear([[maybe_unused]] EventArgs& args)
{
	Clear();
	return true;
}

bool DebugGizmos::Command_DebugRenderToggle([[maybe_unused]] EventArgs& args)
{
	s_isVisible = !s_isVisible;
	return true;
}
