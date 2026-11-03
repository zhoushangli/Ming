#pragma once

#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

#include <string>
#include <vector>

class VBoxContainer;
class Label;
class PanelContainer;
class ScrollContainer;
class TextureRect;

class InspectorPanel final : public EditorPanel
{
	MCLASS(InspectorPanel, EditorPanel);

private:
	struct PropertyEntry
	{
		InspectorProperty* m_property = nullptr;
		MethodBind const*  m_getter   = nullptr;
		int                m_section  = -1;
	};

	struct PropertySection
	{
		std::string m_name;
		Button*     m_heading   = nullptr;
		bool        m_collapsed = false;
	};

	struct PropertyGroup
	{
		std::string                  m_className;
		Control*                     m_header = nullptr;
		PanelContainer*              m_bar    = nullptr;
		Label*                       m_title  = nullptr;
		TextureRect*                 m_icon   = nullptr;
		std::vector<PropertyEntry>   m_properties;
		std::vector<PropertySection> m_sections;
	};

public:
	InspectorPanel();
	~InspectorPanel() override;
	void     FinishInput(EditorUIContext& context);
	void     FinishEditing();
	Control* GetContentControl() const;

private:
	void OnInitialize() override;
	void OnClose() override;
	void OnRender(EditorUIContext& context) override;

	void BuildInheritanceChain(std::string const& className);
	void RebuildProperties(EditorUIContext& context);
	void ClearProperties();
	void RefreshValues();
	void FilterProperties();
	void CommitValue(
		std::string const& propertyName,
		ObjectID           objectID,
		MethodBind const*  setter,
		MethodBind const*  getter,
		Variant const&     value);

private:
	ObjectID                   m_cachedObjectID;
	VBoxContainer*             m_content    = nullptr;
	Label*                     m_empty      = nullptr;
	LineEdit*                  m_filter     = nullptr;
	ScrollContainer*           m_scroll     = nullptr;
	PanelContainer*            m_objectBar  = nullptr;
	Label*                     m_objectName = nullptr;
	TextureRect*               m_objectIcon = nullptr;
	SceneTree*                 m_sceneTree  = nullptr;
	std::vector<std::string>   m_inheritanceChain;
	std::vector<PropertyGroup> m_propertyGroups;
};
