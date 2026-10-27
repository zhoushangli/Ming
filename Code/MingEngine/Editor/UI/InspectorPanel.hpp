#pragma once

#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Editor/UI/EditorControlHost.hpp"

#include <string>
#include <vector>

class InspectorPanel final : public EditorPanel
{
public:
	InspectorPanel();
	~InspectorPanel() override;
	EditorControlHost&       GetHost() { return m_host; }
	EditorControlHost const& GetHost() const { return m_host; }
	void                     FinishInput(EditorUIContext& context);
	void RenderNative(EditorUIContext& context, Vector2 origin, Vector2 size);

private:
	struct PropertyEntry
	{
		InspectorProperty* m_property = nullptr;
		MethodBind const*  m_getter   = nullptr;
	};

	struct PropertyGroup
	{
		std::string                m_className;
		std::vector<PropertyEntry> m_properties;
	};

	void OnRender(EditorUIContext& context) override;

	void BuildInheritanceChain(std::string const& className);
	void RebuildProperties(EditorUIContext& context);
	void ClearProperties();
	void RefreshValues();
	void CommitValue(ObjectID objectID, MethodBind const* setter, MethodBind const* getter, Variant const& value);

private:
	ObjectID                   m_cachedObjectID;
	EditorControlHost          m_host;
	SceneTree*                 m_sceneTree = nullptr;
	std::vector<std::string>   m_inheritanceChain;
	std::vector<PropertyGroup> m_propertyGroups;
};
