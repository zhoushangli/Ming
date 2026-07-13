#pragma once

#include <string>

class EditorData
{
public:
	std::string m_currentScenePath;
	bool        m_isSceneDirty = false;
};
