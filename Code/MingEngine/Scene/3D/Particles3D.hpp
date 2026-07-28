#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Scene/3D/VisualInstance3D.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include <string>

class VertexBuffer;

class Particles3D : public VisualInstance3D
{
public:
	enum class EmitMode
	{
		Continuous,
		Burst,
		Count
	};

	enum class SimulationSpace
	{
		Local,
		World,
		Count
	};

	enum class EmitShape
	{
		Point,
		Disc,
		Count
	};

	struct Particle3D
	{
		bool IsEnabled() const { return m_lifetimeRemaining > 0.f; }

		// position and velocity are in simulation space
		Vec3 m_position = Vec3::Zero;
		Vec3 m_velocity = Vec3::Zero;

		EulerAngles m_orientation = EulerAngles::Zero;

		float m_lifetimeRemaining = 0.f;
		float m_totalLifetime = 0.f;
	};

public:
	Particles3D(std::string const &xmlFilePath);
	~Particles3D() override;

	void OnNotification(int notification);
	RenderRequest SubmitRenderRequest() const override;

	void SpawnNewParticle(Vec3 const &position);
	void RebuildParticleVerts();
	void LoadFromXML(std::string const &xmlFilePath);

	void StopAndDestroyWhenEmpty();

	Matrix4x4 GetSimulationToWorld() const;
	Matrix4x4 GetWorldToSimulation() const;
	Vec3 WorldToSimulation(Vec3 const &worldPos) const;
	Vec3 SimulationToWorld(Vec3 const &simPos) const;

public:
	// Inner Variables
	float m_spawnTimer = 0.f;
	float m_spawnInterval = 0.f;
	bool m_isEmitting = true;
	bool m_destroyWhenEmpty = false;
	Vec3 m_lastEmitterPos = Vec3::Zero;

	std::vector<Particle3D> m_particles;

	// This will be generated each frame based on particles' position, size, color, and camera orientation
	Ref<TextureResource> m_particleTextureRef;
	std::vector<Vertex> m_particleVerts;
	mutable VertexBuffer *m_particleVertexBuffer = nullptr;

	// Emitter properties
	std::string m_name;
	EmitMode m_emitMode = EmitMode::Continuous;
	EmitShape m_emitShape = EmitShape::Point;
	SimulationSpace m_simulationSpace = SimulationSpace::Local;
	Math::BillboardType m_billboardType = Math::BillboardType::FULL_FACING;
	VirtualPath m_imagePath;
	float m_lifetime = 1.f;
	float m_emitRadius = 0.f;
	float m_startSize = 1.f;
	float m_endSize = 1.f;
	Color m_startColor = Color::White;
	Color m_endColor = Color::White;
	EulerAngles m_orientationMin = EulerAngles::Zero;
	EulerAngles m_orientationMax = EulerAngles::Zero;

	Vec3 m_velocityDirection = Vec3::Zero;
	FloatRange m_velocityRange = FloatRange::Zero;
	float m_coneHalfAngleDegrees = 0.f;

	// Continuous mode properties
	float m_spawnRate = 10.f;
	float m_maxParticles = 100.f;

	// Burst mode properties
	int m_burstCount = 20;
};
