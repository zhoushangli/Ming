#pragma once

namespace JPH
{
class BroadPhaseLayerInterfaceTable;
class JobSystemSingleThreaded;
class ObjectLayerPairFilterTable;
class ObjectVsBroadPhaseLayerFilterTable;
class PhysicsSystem;
class TempAllocatorMalloc;
}

class PhysicsServer
{
public:
	PhysicsServer() = default;
	~PhysicsServer();

	PhysicsServer(PhysicsServer const&)            = delete;
	PhysicsServer& operator=(PhysicsServer const&) = delete;
	PhysicsServer(PhysicsServer&&)                 = delete;
	PhysicsServer& operator=(PhysicsServer&&)      = delete;

	bool Startup();
	void Shutdown();
	bool Step(float fixedDeltaSeconds);

	// Simulate a sphere falling onto a floor in this server's Z-up world.
	// e.g. server.Startup(); bool passed = server.RunTest(); server.Shutdown();
	bool RunTest();

private:
	JPH::PhysicsSystem*                      m_physicsSystem     = nullptr;
	JPH::BroadPhaseLayerInterfaceTable*      m_broadPhaseLayers  = nullptr;
	JPH::ObjectLayerPairFilterTable*         m_objectLayerFilter = nullptr;
	JPH::ObjectVsBroadPhaseLayerFilterTable* m_broadPhaseFilter  = nullptr;
	JPH::JobSystemSingleThreaded*            m_jobSystem         = nullptr;
	JPH::TempAllocatorMalloc*               m_tempAllocator     = nullptr;
};
