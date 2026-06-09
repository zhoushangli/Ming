#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Scene/Core/ClassDatabase.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"
#include "MingEngine/Scene/Core/Variant.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class InspectorPanel final : public EditorPanel
{
public:
	InspectorPanel();

private:
	void OnRender(EditorUIContext& context) override;

	void BuildInheritanceChain(std::string const& className);
	void RefreshPropertyValues(EditorUIContext& context, std::string const& className);
	void RenderTabBar();
	void RenderProperty(
		ClassDatabase::PropertyInfo const& prop,
		Variant& value,
		EditorUIContext& context,
		std::string const& className,
		bool showLabel = true);
	static std::string SnakeToTitle(std::string const& snake);

private:
	NodeHandle m_cachedHandle;
	std::vector<std::string> m_inheritanceChain;
	size_t m_activeTabIndex = 0;
	std::unordered_map<std::string, std::vector<Variant>> m_propertyValuesByClass;
};
