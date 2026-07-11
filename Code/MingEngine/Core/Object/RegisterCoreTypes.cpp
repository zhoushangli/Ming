#include "MingEngine/Scene/RegisterAllTypes.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/ScriptLoader.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"

#define BIND_GLOBAL_METHOD(namespaceName, method)                                                                      \
	ClassDatabase::BindGlobalMethod(#namespaceName, #method, &namespaceName::method);

namespace
{
ScriptLoader* scriptLoader = new ScriptLoader();
} // namespace

#pragma region Core

void RegisterCoreTypes()
{
	// Root
	ClassDatabase::RegisterRootClass<Object>();

	// Core types
	ClassDatabase::RegisterClass<RefCounted>(false);
	ClassDatabase::RegisterClass<Resource>(false);
	ClassDatabase::RegisterClass<Script>();
	ClassDatabase::RegisterClass<ResourceFormatImporter>(false);
	ClassDatabase::RegisterClass<ResourceFormatLoader>(false);
	ClassDatabase::RegisterClass<ResourceFormatSaver>(false);

	// Script loader
	ClassDatabase::RegisterClass<ScriptLoader>();
	ResourceLoader::AddLoader(Ref<ScriptLoader>(scriptLoader));

	// --- Global math methods ---

	// Basic Math
	// TODO: Abs is an overridable function, need more design here
	BIND_GLOBAL_METHOD(Math, Sign);

	// Angles & Trigonometry
	BIND_GLOBAL_METHOD(Math, ConvertDegreesToRadians);
	BIND_GLOBAL_METHOD(Math, ConvertRadiansToDegrees);
	BIND_GLOBAL_METHOD(Math, CosDegrees);
	BIND_GLOBAL_METHOD(Math, SinDegrees);
	BIND_GLOBAL_METHOD(Math, Atan2Degrees);
	BIND_GLOBAL_METHOD(Math, GetShortestAngularDispDegrees);
	BIND_GLOBAL_METHOD(Math, GetTurnedTowardDegrees);

	// Distance
	BIND_GLOBAL_METHOD(Math, GetDistance2D);
	BIND_GLOBAL_METHOD(Math, GetDistanceSquared2D);
	BIND_GLOBAL_METHOD(Math, GetDistance3D);
	BIND_GLOBAL_METHOD(Math, GetDistanceXY3D);
	BIND_GLOBAL_METHOD(Math, GetDistanceSquared3D);
	BIND_GLOBAL_METHOD(Math, GetDistanceXYSquared3D);

	// Overlap
	BIND_GLOBAL_METHOD(Math, DoCapsulesOverlap3D);

	// Smoothing
	BIND_GLOBAL_METHOD(Math, SmoothStart2);
	BIND_GLOBAL_METHOD(Math, SmoothStart3);
	BIND_GLOBAL_METHOD(Math, SmoothStart4);
	BIND_GLOBAL_METHOD(Math, SmoothStart5);
	BIND_GLOBAL_METHOD(Math, SmoothStart6);
	BIND_GLOBAL_METHOD(Math, SmoothStop2);
	BIND_GLOBAL_METHOD(Math, SmoothStop3);
	BIND_GLOBAL_METHOD(Math, SmoothStop4);
	BIND_GLOBAL_METHOD(Math, SmoothStop5);
	BIND_GLOBAL_METHOD(Math, SmoothStop6);
	BIND_GLOBAL_METHOD(Math, SmoothStep3);
	BIND_GLOBAL_METHOD(Math, SmoothStep5);
	BIND_GLOBAL_METHOD(Math, Hesitate3);
	BIND_GLOBAL_METHOD(Math, Hesitate5);

	// Range / Mapping
	BIND_GLOBAL_METHOD(Math, GetFractionWithinRange);
	BIND_GLOBAL_METHOD(Math, RangeMap);
	BIND_GLOBAL_METHOD(Math, RangeMapClamped);

	// Clamp / Round
	BIND_GLOBAL_METHOD(Math, GetClampedZeroToOne);
	BIND_GLOBAL_METHOD(Math, RoundDownToInt);

	// Dot / Cross / Projection
	BIND_GLOBAL_METHOD(Math, DotProduct2D);
	BIND_GLOBAL_METHOD(Math, DotProduct3D);
	BIND_GLOBAL_METHOD(Math, DotProduct4D);
	BIND_GLOBAL_METHOD(Math, CrossProduct2D);
	BIND_GLOBAL_METHOD(Math, CrossProduct3D);
	BIND_GLOBAL_METHOD(Math, GetProjectedLength2D);
	BIND_GLOBAL_METHOD(Math, GetProjectedVector2D);
	BIND_GLOBAL_METHOD(Math, GetProjectedVector3D);
	BIND_GLOBAL_METHOD(Math, GetAngleDegreesBetweenVectors2D);

	// Push Out
	BIND_GLOBAL_METHOD(Math, PushDiscOutOfFixedPoint2D);

	// Point Inside
	BIND_GLOBAL_METHOD(Math, IsPointInsideAABB2D);
	BIND_GLOBAL_METHOD(Math, IsPointInsideOBB2D);
	BIND_GLOBAL_METHOD(Math, IsPointInsideOrientedSector2D);
	BIND_GLOBAL_METHOD(Math, IsPointInsideDirectedSector2D);

	// Nearest Point
	BIND_GLOBAL_METHOD(Math, GetNearestPointOnAABB2D);
	BIND_GLOBAL_METHOD(Math, GetNearestPointOnOBB2D);
	BIND_GLOBAL_METHOD(Math, GetNearestPointOnLine3D);
	BIND_GLOBAL_METHOD(Math, GetNearestPointOnCapsule3D);

	// Bezier
	BIND_GLOBAL_METHOD(Math, ComputeCubicBezier1D);
	BIND_GLOBAL_METHOD(Math, ComputeQuinticBezier1D);
	BIND_GLOBAL_METHOD(Math, ComputeCubicBezier2D);
	BIND_GLOBAL_METHOD(Math, ComputeQuinticBezier2D);
	BIND_GLOBAL_METHOD(Math, ComputeCubicBezier3D);
	BIND_GLOBAL_METHOD(Math, ComputeQuinticBezier3D);

	// Billboard
	// BIND_GLOBAL_METHOD(Math, GetBillboardTransform);
}

#pragma endregion
