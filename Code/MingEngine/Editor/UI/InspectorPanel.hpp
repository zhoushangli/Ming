#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include <string>
#include <vector>

class InspectorPanel final : public EditorPanel
{
public:
	InspectorPanel();
	~InspectorPanel() override;

private:
	void OnRender(EditorUIContext& context) override;

	void BuildInheritanceChain(std::string const& className);
	void RebuildProperties(EditorUIContext& context);
	void RenderTabBar();

private:
	NodeHandle m_cachedHandle;
	std::vector<std::string> m_inheritanceChain;
	size_t m_activeTabIndex = 0;
	std::vector<InspectorProperty*> m_properties;
};
