#pragma once
#include <chrono>

#include "MingEngine/Engine/File/VirtualPath.hpp"

class EditorData
{
public:
	VirtualPath m_currentScenePath;
	bool        m_isSceneDirty = false;
	std::chrono::steady_clock::time_point m_sceneActionTime = std::chrono::steady_clock::now();
	bool m_sceneWasSaved = false;
};
