#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

class FileSystemPanel final : public EditorPanel
{
public:
	FileSystemPanel();

private:
	void OnRender(EditorUIContext& context) override;

private:
	char m_filter[64] = {};
};
