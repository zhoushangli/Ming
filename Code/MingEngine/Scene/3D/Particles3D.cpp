#include "MingEngine/Scene/3D/Particles3D.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/RandomNumberGenerator.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Core/XmlUtils.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

using namespace Math;

namespace
{
Particles3D::EmitMode ParseXmlAttribute(
	XmlElement const& element, char const* attributeName, Particles3D::EmitMode defaultValue)
{
	std::string value = ::ParseXmlAttribute(element, attributeName, "");
	if (value.empty())
	{
		return defaultValue;
	}

	if (value == "continuous" || value == "Continuous")
	{
		return Particles3D::EmitMode::Continuous;
	}

	if (value == "burst" || value == "Burst")
	{
		return Particles3D::EmitMode::Burst;
	}

	ERROR_RECOVERABLE(Stringf(
		"Unknown particle emitType '%s' for attribute '%s'; using default emit mode instead.",
		value.c_str(),
		attributeName));
	return defaultValue;
}

Particles3D::SimulationSpace ParseXmlAttribute(
	XmlElement const& element, char const* attributeName, Particles3D::SimulationSpace defaultValue)
{
	std::string value = ::ParseXmlAttribute(element, attributeName, "");
	if (value.empty())
	{
		return defaultValue;
	}

	if (value == "local" || value == "Local")
	{
		return Particles3D::SimulationSpace::Local;
	}

	if (value == "world" || value == "World")
	{
		return Particles3D::SimulationSpace::World;
	}

	ERROR_RECOVERABLE(Stringf(
		"Unknown particle simulationSpace '%s' for attribute '%s'; using default simulation space instead.",
		value.c_str(),
		attributeName));
	return defaultValue;
}

Particles3D::EmitShape ParseXmlAttribute(
	XmlElement const& element, char const* attributeName, Particles3D::EmitShape defaultValue)
{
	std::string value = ::ParseXmlAttribute(element, attributeName, "");
	if (value.empty())
	{
		return defaultValue;
	}

	if (value == "point" || value == "Point")
	{
		return Particles3D::EmitShape::Point;
	}

	if (value == "disc" || value == "Disc")
	{
		return Particles3D::EmitShape::Disc;
	}

	ERROR_RECOVERABLE(Stringf(
		"Unknown particle emitShape '%s' for attribute '%s'; using default emit shape instead.",
		value.c_str(),
		attributeName));
	return defaultValue;
}

BillboardType ParseXmlAttribute(XmlElement const& element, char const* attributeName, BillboardType defaultValue)
{
	std::string value = ::ParseXmlAttribute(element, attributeName, "");
	if (value.empty())
	{
		return defaultValue;
	}

	if (value == "NONE" || value == "None" || value == "none")
	{
		return BillboardType::NONE;
	}

	if (value == "WORLD_UP_FACING" || value == "WorldUpFacing" || value == "worldUpFacing")
	{
		return BillboardType::WORLD_UP_FACING;
	}

	if (value == "FULL_FACING" || value == "FullFacing" || value == "fullFacing")
	{
		return BillboardType::FULL_FACING;
	}

	ERROR_RECOVERABLE(Stringf(
		"Unknown particle billboardType '%s' for attribute '%s'; using default billboard type instead.",
		value.c_str(),
		attributeName));
	return defaultValue;
}

EulerAngles ParseEulerAnglesAttribute(
	XmlElement const& element, char const* attributeName, EulerAngles const& defaultValue)
{
	Vector3 const angles = ::ParseXmlAttribute(
		element,
		attributeName,
		Vector3(defaultValue.m_yawDegrees, defaultValue.m_pitchDegrees, defaultValue.m_rollDegrees));
	return EulerAngles(angles.x, angles.y, angles.z);
}

EulerAngles GetRandomOrientationInRange(EulerAngles const& minOrientation, EulerAngles const& maxOrientation)
{
	RandomNumberGenerator& rng = RandomNumberGenerator::Get();
	return EulerAngles(
		rng.RollRandomFloatInRange(minOrientation.m_yawDegrees, maxOrientation.m_yawDegrees),
		rng.RollRandomFloatInRange(minOrientation.m_pitchDegrees, maxOrientation.m_pitchDegrees),
		rng.RollRandomFloatInRange(minOrientation.m_rollDegrees, maxOrientation.m_rollDegrees));
}

Vector3 GetRandomDirectionInCone(Vector3 const& forwardDirection, float coneHalfAngleDegrees)
{
	Vector3 forward = forwardDirection.GetNormalized();
	if (forward.GetLengthSquared() <= 1e-5f)
	{
		return Vector3::Forward;
	}

	if (coneHalfAngleDegrees <= 0.f)
	{
		return forward;
	}

	RandomNumberGenerator& rng            = RandomNumberGenerator::Get();
	float                  yawDegrees     = rng.RollRandomFloatInRange(-coneHalfAngleDegrees, coneHalfAngleDegrees);
	float                  pitchDegrees   = rng.RollRandomFloatInRange(-coneHalfAngleDegrees, coneHalfAngleDegrees);
	Vector3                localDirection = Vector3::MakeFromPolarDegrees(pitchDegrees, yawDegrees);

	EulerAngles coneToWorldOrientation = EulerAngles::MakeFromForward(forward);
	Matrix4x4   coneToWorld            = coneToWorldOrientation.GetAsMatrix_IFwd_JLeft_KUp();
	Vector3     coneDirection          = coneToWorld.TransformDirection3D(localDirection);

	return coneDirection.GetNormalized();
}

Vector3 GetRandomPointInDisc(float radius)
{
	if (radius <= 0.f)
	{
		return Vector3::Zero;
	}

	RandomNumberGenerator& rng          = RandomNumberGenerator::Get();
	float const            angleDegrees = rng.RollRandomFloatInRange(0.f, 360.f);
	float const            distance     = radius * sqrtf(rng.RollRandomFloatZeroToOne());
	return Vector3(distance * CosDegrees(angleDegrees), distance * SinDegrees(angleDegrees), 0.f);
}
} // namespace

Particles3D::Particles3D(std::string const& xmlFilePath)
{
	LoadFromXML(xmlFilePath);

	m_particles.reserve((size_t)m_maxParticles);
	m_particleVerts.reserve((size_t)m_maxParticles * 16);

	if (m_imagePath.IsValid())
	{
		Ref<Resource> loaded = ResourceLoader::Load(m_imagePath);
		m_particleTextureRef = Ref<TextureResource>(loaded);
	}
	m_spawnInterval = 1.f / m_spawnRate;
}

Particles3D::~Particles3D()
{
	delete m_particleVertexBuffer;
	m_particleVertexBuffer = nullptr;
}

void Particles3D::LoadFromXML(std::string const& xmlFilePath)
{
	XmlDocument doc;
	XmlError    loadResult = doc.LoadFile(xmlFilePath.c_str());
	ASSERT_OR_DIE(
		loadResult == tinyxml2::XML_SUCCESS,
		Stringf("Failed to load particle xml file: %s\n", xmlFilePath.c_str()));

	XmlElement* root = doc.RootElement();
	ASSERT_OR_DIE(root != nullptr, Stringf("Particle xml file is empty: %s\n", xmlFilePath.c_str()));

	XmlElement const* emitterElement = root->FirstChildElement("Emitter");
	XmlElement const* visualElement  = root->FirstChildElement("Visual");
	XmlElement const* motionElement  = root->FirstChildElement("Motion");
	emitterElement                   = emitterElement != nullptr ? emitterElement : root;
	visualElement                    = visualElement != nullptr ? visualElement : root;
	motionElement                    = motionElement != nullptr ? motionElement : root;

	m_name         = ParseXmlAttribute(*root, "name", m_name);
	m_emitMode     = ParseXmlAttribute(*emitterElement, "emitType", m_emitMode);
	m_emitShape    = ParseXmlAttribute(*emitterElement, "emitShape", m_emitShape);
	m_emitRadius   = ParseXmlAttribute(*emitterElement, "emitRadius", m_emitRadius);
	m_lifetime     = ParseXmlAttribute(*emitterElement, "lifetime", m_lifetime);
	m_spawnRate    = ParseXmlAttribute(*emitterElement, "spawnRate", m_spawnRate);
	m_maxParticles = ParseXmlAttribute(*emitterElement, "maxParticles", m_maxParticles);
	m_burstCount   = ParseXmlAttribute(*emitterElement, "burstCount", m_burstCount);

	m_simulationSpace           = ParseXmlAttribute(*visualElement, "simulationSpace", m_simulationSpace);
	m_billboardType             = ParseXmlAttribute(*visualElement, "billboardType", m_billboardType);
	std::string const imagePath = ParseXmlAttribute(*visualElement, "imagePath", m_imagePath.GetString());
	if (!imagePath.empty())
	{
		VirtualPath::TryParse(imagePath, m_imagePath);
	}
	m_startSize      = ParseXmlAttribute(*visualElement, "startSize", m_startSize);
	m_endSize        = ParseXmlAttribute(*visualElement, "endSize", m_endSize);
	m_startColor     = ParseXmlAttribute(*visualElement, "startColor", m_startColor);
	m_endColor       = ParseXmlAttribute(*visualElement, "endColor", m_endColor);
	m_orientationMin = ParseEulerAnglesAttribute(*visualElement, "orientationMin", m_orientationMin);
	m_orientationMax = ParseEulerAnglesAttribute(*visualElement, "orientationMax", m_orientationMax);

	m_velocityDirection    = ParseXmlAttribute(*motionElement, "velocityDirection", m_velocityDirection);
	m_velocityRange        = ParseXmlAttribute(*motionElement, "velocityRange", m_velocityRange);
	m_coneHalfAngleDegrees = ParseXmlAttribute(*motionElement, "coneHalfAngleDegrees", m_coneHalfAngleDegrees);
}

void Particles3D::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Ready:
	{
		m_lastEmitterPos = GetWorldPosition();

		if (m_emitMode == EmitMode::Burst)
		{
			for (int i = 0; i < m_burstCount; i++)
			{
				SpawnNewParticle(GetWorldPosition());
			}
			StopAndDestroyWhenEmpty();
		}
		break;
	}
	case Notification_Process:
	{
		float      deltaSeconds = 0.f;
		SceneTree* sceneTree    = GetSceneTree();
		if (sceneTree != nullptr)
		{
			deltaSeconds = sceneTree->GetDeltaSeconds();
		}
		if (m_isEmitting)
		{
			if (m_emitMode == EmitMode::Continuous)
			{
				float oldTimer = m_spawnTimer;
				m_spawnTimer += deltaSeconds;

				float spawnTimeInFrame = m_spawnInterval - oldTimer;
				while (m_spawnTimer >= m_spawnInterval)
				{
					float   fraction         = spawnTimeInFrame / deltaSeconds;
					Vector3 particlePosition = InterpolateClamped(m_lastEmitterPos, GetWorldPosition(), fraction);
					SpawnNewParticle(particlePosition);

					m_spawnTimer -= m_spawnInterval;
					spawnTimeInFrame += m_spawnInterval;
				}
			}
		}

		// 1) Spawn new particles based on spawn rate and emitter lifetime
		// 2) Update existing particles' position, size, color based on their velocity, lifetime, and emitter properties
		// 3) Remove particles that have exceeded their lifetime

		bool hasAliveParticles = false;
		for (Particle3D& particle : m_particles)
		{
			if (!particle.IsEnabled())
			{
				continue;
			}

			particle.m_lifetimeRemaining -= deltaSeconds;
			particle.m_position += particle.m_velocity * deltaSeconds;

			hasAliveParticles = true;
		}

		if (m_destroyWhenEmpty && !hasAliveParticles)
		{
			DeleteNode();
			return;
		}

		m_lastEmitterPos = GetWorldPosition();

		RebuildParticleVerts();
		break;
	}
	}
}

RenderRequest Particles3D::SubmitRenderRequest() const
{
	RenderRequest request;
	if (m_particleVerts.empty())
	{
		return request;
	}

	unsigned int const size = static_cast<unsigned int>(m_particleVerts.size() * sizeof(Vertex));
	if (m_particleVertexBuffer == nullptr)
	{
		m_particleVertexBuffer = g_engine->m_renderer->CreateVertexBuffer(size, sizeof(Vertex));
	}
	else if (m_particleVertexBuffer->GetSize() < size)
	{
		m_particleVertexBuffer->Resize(size);
	}
	g_engine->m_renderer->CopyCPUToGPU(m_particleVerts.data(), size, m_particleVertexBuffer);

	request.m_pass         = RenderRequestPass::Opaque;
	request.m_modelToWorld = Matrix4x4::Identity;
	request.m_tint         = Color::White;
	request.m_vertexBuffer = m_particleVertexBuffer;
	request.m_textures[SurfaceTextureSlot::Diffuse] =
		m_particleTextureRef.IsValid() ? m_particleTextureRef->GetGPUTexture() : nullptr;
	request.m_shader         = nullptr;
	request.m_blendMode      = BlendMode::ADDITIVE;
	request.m_depthMode      = DepthMode::READ_WRITE_LESS_EQUAL;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
	request.m_samplerMode    = SamplerMode::POINT_CLAMP;
	return request;
}

void Particles3D::SpawnNewParticle(Vector3 const& worldPosition)
{
	auto InitParticle = [&](Particle3D& particle)
	{
		Vector3 spawnWorldPosition = worldPosition;
		if (m_emitShape == EmitShape::Disc)
		{
			spawnWorldPosition += GetWorldTransform().TransformDirection3D(GetRandomPointInDisc(m_emitRadius));
		}

		particle.m_position    = WorldToSimulation(spawnWorldPosition);
		particle.m_orientation = GetRandomOrientationInRange(m_orientationMin, m_orientationMax);

		Vector3 localRandomDir = GetRandomDirectionInCone(m_velocityDirection, m_coneHalfAngleDegrees);
		Vector3 worldRandomDir = GetWorldTransform().TransformDirection3D(localRandomDir);
		float   speed          = m_velocityRange.GetRandomInRange();
		particle.m_velocity    = GetWorldToSimulation().TransformDirection3D(worldRandomDir) * speed;

		particle.m_lifetimeRemaining = m_lifetime;
		particle.m_totalLifetime     = m_lifetime;
	};

	bool findSlot = false;
	for (Particle3D& particle : m_particles)
	{
		if (!particle.IsEnabled())
		{
			findSlot = true;
			InitParticle(particle);
			break;
		}
	}

	if (!findSlot && m_particles.size() < (size_t)m_maxParticles)
	{
		Particle3D newParticle;
		InitParticle(newParticle);
		m_particles.push_back(newParticle);
	}
}

void Particles3D::RebuildParticleVerts()
{
	SceneTree* sceneTree    = GetSceneTree();
	Camera3D*  activeCamera = sceneTree != nullptr ? sceneTree->GetWorldCamera() : nullptr;
	if (activeCamera == nullptr)
	{
		return;
	}
	Matrix4x4 cameraTransform = activeCamera->GetWorldTransform();

	m_particleVerts.clear();

	for (Particle3D const& particle : m_particles)
	{
		if (!particle.IsEnabled())
		{
			continue;
		}

		float lifeProgress = 1.f - particle.m_lifetimeRemaining / particle.m_totalLifetime;

		float size  = Interpolate(m_startSize, m_endSize, lifeProgress);
		Color color = Interpolate(m_startColor, m_endColor, lifeProgress);

		Vector3 bottomLeft  = Vector3(0.f, -size, -size);
		Vector3 bottomRight = Vector3(0.f, size, -size);
		Vector3 topRight    = Vector3(0.f, size, size);
		Vector3 topLeft     = Vector3(0.f, -size, size);

		AddVertsForQuad3D(m_particleVerts, bottomLeft, bottomRight, topRight, topLeft, color);

		Matrix4x4 particleTransform;
		if (m_billboardType == BillboardType::NONE)
		{
			Matrix4x4 particleLocalTransform = particle.m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
			particleLocalTransform.SetTranslation3D(particle.m_position);

			particleTransform = GetSimulationToWorld();
			particleTransform.Append(particleLocalTransform);
		}
		else
		{
			Vector3 particleWorldPosition = SimulationToWorld(particle.m_position);
			particleTransform = GetBillboardTransform(m_billboardType, cameraTransform, particleWorldPosition);
		}

		for (size_t i = m_particleVerts.size() - 6; i < m_particleVerts.size(); ++i)
		{
			m_particleVerts[i].m_position = particleTransform.TransformPosition3D(m_particleVerts[i].m_position);
		}
	}
}

void Particles3D::StopAndDestroyWhenEmpty()
{
	m_isEmitting       = false;
	m_destroyWhenEmpty = true;
}

Matrix4x4 Particles3D::GetSimulationToWorld() const
{
	if (m_simulationSpace == SimulationSpace::Local)
	{
		return GetWorldTransform();
	}
	else
	{
		return Matrix4x4::Identity;
	}
}

Matrix4x4 Particles3D::GetWorldToSimulation() const
{
	if (m_simulationSpace == SimulationSpace::Local)
	{
		return GetWorldTransform().GetOrthonormalInverse();
	}
	else
	{
		return Matrix4x4::Identity;
	}
}

Vector3 Particles3D::WorldToSimulation(Vector3 const& worldPos) const
{
	Matrix4x4 worldToSim = GetWorldToSimulation();
	return worldToSim.TransformPosition3D(worldPos);
}

Vector3 Particles3D::SimulationToWorld(Vector3 const& simPos) const
{
	Matrix4x4 simToWorld = GetSimulationToWorld();
	return simToWorld.TransformPosition3D(simPos);
}
