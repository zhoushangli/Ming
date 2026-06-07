#pragma once

#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include "MingEngine/Engine/Math/RaycastUtils.hpp"
#include "MingEngine/Engine/Math/Vec2.hpp"
#include "MingEngine/Engine/Math/Vec3.hpp"

class CameraContext;

struct GameRaycastResult : public RaycastResult3D
{
	NodeHandle m_hitNodeHandle  = NodeHandle::Invalid;
	NodeHandle m_hitOwnerHandle = NodeHandle::Invalid;
};

struct RaycastInfo
{
	Vec3  m_startPos;
	Vec3  m_forwardNormal;
	float m_maxLength = 1.f;

	NodeHandle m_ignoreNodeHandle = NodeHandle::Invalid;
};

RaycastInfo
BuildRaycastFromMouse(CameraContext const& camera, Vec2 const& clientPos, Vec2 const& clientDimensions, float maxLength);
