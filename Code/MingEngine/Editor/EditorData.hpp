#pragma once
#include <chrono>

#include "MingEngine/Core/IO/VirtualPath.hpp"

class EditorData
{
public:
	VirtualPath                           m_currentScenePath;
	std::chrono::steady_clock::time_point m_sceneActionTime = std::chrono::steady_clock::now();
	bool                                  m_sceneWasSaved = false;
};
