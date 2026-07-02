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
	struct PropertyGroup
	{
		std::string                     m_className;
		std::vector<InspectorProperty*> m_properties;
	};

	void OnRender(EditorUIContext& context) override;

	void BuildInheritanceChain(std::string const& className);
	void RebuildProperties(EditorUIContext& context);

private:
	NodeHandle m_cachedHandle;
	std::vector<std::string> m_inheritanceChain;
	std::vector<PropertyGroup> m_propertyGroups;
};
