#pragma once

#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorProperty.hpp"

#include <string>
#include <vector>

class VBoxContainer;
class SubViewportContainer;
class MeshInstance3D;
class Label;
class PanelContainer;
class ScrollContainer;
class TextureRect;

class InspectorPanel final : public EditorPanel
{
	MCLASS(InspectorPanel, EditorPanel);

private:
	struct PropertyEdit
	{
		ObjectID           m_target = ObjectID::Invalid;
		ObjectID           m_scene  = ObjectID::Invalid;
		std::string        m_name;
		InspectorProperty* m_field  = nullptr;
		Variant            m_original;
		Variant            m_originalState;
		uint64_t           m_revision = 0;
		Variant            m_last;
		bool               m_applying = false;
	};

	struct PropertyEntry
	{
		InspectorProperty* m_property = nullptr;
		bool               m_dynamic = false;
		std::string        m_name;
		int                m_section  = -1;
	};

	struct PropertySection
	{
		std::string    m_name;
		VBoxContainer* m_root      = nullptr;
		Button*        m_heading   = nullptr;
		Control*       m_content   = nullptr;
		VBoxContainer* m_fields    = nullptr;
		bool           m_collapsed = false;
		bool           m_dynamic = false;
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
	void EditResource(Ref<Resource> const& resource);
	void GoBack();
	void GoForward();
	bool SaveResource();
	void SaveResourceAs();
	ObjectID GetUndoHistory() const;

	void     FinishInput(EditorUIContext& context);
	void     FinishEditing();
	Control* GetContentControl() const;

private:
	Object* GetTarget(ObjectID id) const;
	void BuildMaterialPreview();
	void OnNotification(int notification);
	void OnInitialize() override;
	void OnClose() override;
	void OnRender(EditorUIContext& context) override;

	void BuildInheritanceChain(std::string const& className);
	void RebuildProperties(EditorUIContext& context, bool dynamicOnly = false);
	void ClearProperties();
	void RefreshValues();
	void FilterProperties();
	void BeginPropertyEdit(
		std::string const& name,
		ObjectID           id,
		InspectorProperty* field);
	void PreviewPropertyValue(Variant const& value);
	void EndPropertyEdit(bool accept);
	void CommitValue(
		std::string const& propertyName,
		ObjectID           objectID,
		Variant const&     value,
		Variant const*     original = nullptr);
	void RevertProperty(std::string const& name, ObjectID id);

private:
	Ref<Resource>              m_resource;
	std::vector<Ref<Resource>> m_resourceHistory;
	std::vector<Ref<Resource>> m_resourceForwardHistory;
	ObjectID                   m_sceneSelection = ObjectID::Invalid;
	Control*                   m_resourceActions = nullptr;
	Control*                   m_resourceTools = nullptr;
	Button*                    m_resourceBack = nullptr;
	Button*                    m_resourceForward = nullptr;
	Control*                   m_previewHost = nullptr;
	SubViewportContainer*      m_preview = nullptr;
	MeshInstance3D*            m_previewMesh = nullptr;
	TextureRect*              m_preview2D = nullptr;

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
	PropertyEdit               m_propertyEdit;
	uint64_t                   m_propertyListRevision = 0;
};
