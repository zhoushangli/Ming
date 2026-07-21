#pragma once

#include "MingEngine/Engine/File/VirtualPath.hpp"

class EditorData
{
public:
	VirtualPath m_currentScenePath;
	bool        m_isSceneDirty = false;
};
