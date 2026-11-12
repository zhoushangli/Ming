#include "MingEngine/Engine/Physics/PhysicsServer.hpp"

#define JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
#define JPH_DEBUG_RENDERER
#define JPH_PROFILE_ENABLED
#define JPH_OBJECT_STREAM
#define JPH_USE_AVX2
#define JPH_USE_AVX
#define JPH_USE_SSE4_1
#define JPH_USE_SSE4_2
#define JPH_USE_LZCNT
#define JPH_USE_TZCNT
#define JPH_USE_F16C
#define JPH_USE_FMADD

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include <cstdarg>
#include <cmath>
#include <cstdio>

#pragma comment(lib, "ThirdParty/JoltPhysics/Lib/x64/Debug/Jolt.lib")

namespace
{
void TraceJolt(char const* format, ...)
{
	va_list arguments;
	va_start(arguments, format);
	std::vfprintf(stderr, format, arguments);
	va_end(arguments);
	std::fputc('\n', stderr);
}
}

PhysicsServer::~PhysicsServer()
{
	Shutdown();
}

bool PhysicsServer::Startup()
{
	if (m_physicsSystem != nullptr)
	{
		return true;
	}

	if (JPH::Factory::sInstance != nullptr)
	{
		return false;
	}

	JPH::Trace = TraceJolt;
	if (!JPH::VerifyJoltVersionID())
	{
		return false;
	}

	// 1) Initialize Jolt before allocating its objects.
	JPH::RegisterDefaultAllocator();
	JPH::Factory::sInstance = new JPH::Factory();
	JPH::RegisterTypes();

	// 2) Use one collision layer and the existing Jolt filter implementations.
	m_objectLayerFilter = new JPH::ObjectLayerPairFilterTable(1);
	m_objectLayerFilter->EnableCollision(0, 0);
	m_broadPhaseLayers = new JPH::BroadPhaseLayerInterfaceTable(1, 1);
	m_broadPhaseLayers->MapObjectToBroadPhaseLayer(0, JPH::BroadPhaseLayer(0));
	m_broadPhaseFilter = new JPH::ObjectVsBroadPhaseLayerFilterTable(
		*m_broadPhaseLayers, 1, *m_objectLayerFilter, 1);

	// 3) Create a small single-threaded world with Ming's Z-up gravity.
	m_jobSystem = new JPH::JobSystemSingleThreaded(JPH::cMaxPhysicsJobs);
	m_tempAllocator = new JPH::TempAllocatorMalloc();
	m_physicsSystem = new JPH::PhysicsSystem();
	m_physicsSystem->Init(128, 0, 128, 128, *m_broadPhaseLayers, *m_broadPhaseFilter, *m_objectLayerFilter);
	m_physicsSystem->SetGravity(JPH::Vec3(0.f, 0.f, -9.81f));
	return true;
}

void PhysicsServer::Shutdown()
{
	if (m_physicsSystem == nullptr)
	{
		return;
	}

	// 1) Destroy the world before its filters and simulation helpers.
	delete m_physicsSystem;
	m_physicsSystem = nullptr;

	delete m_broadPhaseFilter;
	m_broadPhaseFilter = nullptr;
	delete m_broadPhaseLayers;
	m_broadPhaseLayers = nullptr;
	delete m_objectLayerFilter;
	m_objectLayerFilter = nullptr;

	delete m_jobSystem;
	m_jobSystem = nullptr;
	delete m_tempAllocator;
	m_tempAllocator = nullptr;

	// 2) Release Jolt's global types after all owned Jolt objects are gone.
	JPH::UnregisterTypes();
	delete JPH::Factory::sInstance;
	JPH::Factory::sInstance = nullptr;
}

bool PhysicsServer::Step(float fixedDeltaSeconds)
{
	if (m_physicsSystem == nullptr || !std::isfinite(fixedDeltaSeconds) || fixedDeltaSeconds <= 0.f)
	{
		return false;
	}

	return m_physicsSystem->Update(fixedDeltaSeconds, 1, m_tempAllocator, m_jobSystem)
		== JPH::EPhysicsUpdateError::None;
}

bool PhysicsServer::RunTest()
{
	if (m_physicsSystem == nullptr)
	{
		return false;
	}

	// 1) Create a floor whose top is Z=0 and a sphere whose radius is 0.5.
	JPH::BodyInterface& bodies = m_physicsSystem->GetBodyInterface();
	JPH::BodyCreationSettings floorSettings(
		new JPH::BoxShape(JPH::Vec3(10.f, 10.f, 0.5f)),
		JPH::RVec3(0.f, 0.f, -0.5f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, 0);
	JPH::BodyID floor = bodies.CreateAndAddBody(floorSettings, JPH::EActivation::DontActivate);
	if (floor.IsInvalid())
	{
		return false;
	}

	JPH::BodyCreationSettings sphereSettings(
		new JPH::SphereShape(0.5f), JPH::RVec3(0.f, 0.f, 2.f),
		JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, 0);
	JPH::BodyID sphere = bodies.CreateAndAddBody(sphereSettings, JPH::EActivation::Activate);
	if (sphere.IsInvalid())
	{
		bodies.RemoveBody(floor);
		bodies.DestroyBody(floor);
		return false;
	}

	// 2) Advance five simulated seconds and check both falling and resting.
	bool updated = true;
	bool fell = false;
	for (int step = 0; step < 300; ++step)
	{
		if (!Step(1.f / 60.f))
		{
			updated = false;
			break;
		}

		fell = fell || bodies.GetCenterOfMassPosition(sphere).GetZ() < 1.5f;
	}

	float height = static_cast<float>(bodies.GetCenterOfMassPosition(sphere).GetZ());
	float speed = bodies.GetLinearVelocity(sphere).Length();
	bool sleeping = !bodies.IsActive(sphere);
	bool passed = updated && fell && std::abs(height - 0.5f) < 0.05f && speed < 0.1f && sleeping;
	std::printf("[PHYSICS_TEST] height=%.4f speed=%.4f sleeping=%d result=%s\n",
		height, speed, sleeping ? 1 : 0, passed ? "PASS" : "FAIL");

	// 3) Remove and destroy test bodies so the test can run again.
	bodies.RemoveBody(sphere);
	bodies.DestroyBody(sphere);
	bodies.RemoveBody(floor);
	bodies.DestroyBody(floor);
	return passed;
}
