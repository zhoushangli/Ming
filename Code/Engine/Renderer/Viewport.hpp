#pragma once

#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/IntVec2.hpp"

class Camera;

class Viewport
{
public:
	Camera const* m_worldCamera = nullptr;
	Camera const* m_uiCamera    = nullptr;

	// output resolution indicates the size of the render target
	// output rect indicates the portion of the render target to render to
	IntVec2 m_outputResolution = IntVec2::Zero;
	AABB2   m_outputRect       = AABB2::Unit;
};