#pragma once

#include "MingEngine/Core/IO/ResourceImporter.hpp"
#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorProperty.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class VBoxContainer;
class Label;
class TextureRect;
class PanelContainer;
class ScrollContainer;
class PopupPanel;

class ImportPanel final : public EditorPanel
{
	MCLASS(ImportPanel, EditorPanel);

private:
	enum class SwitchChoice
	{
		Reimport,
		Discard,
		Cancel
	};

public:
	ImportPanel();
	~ImportPanel() override;

private:
	void OnInitialize() override;
	void OnRender(EditorUIContext& context) override;
	bool Reimport();
	void RequestSwitch(VirtualPath const& path, int importer = -1);
	void ResolveSwitch(SwitchChoice choice);

	void                        RefreshSelection(VirtualPath const& selectedPath, bool discard = false);
	void                        RefreshImportConfigCache(VirtualPath const& selectedPath);
	void                        ApplyCachedImportOptions();
	int                         FindImporterIndexByClassName(std::string const& importerClassName) const;
	void                        RebuildImportOptionProperties();
	void                        ClearImportOptionProperties();
	Ref<ResourceFormatImporter> GetSelectedImporter() const;
	Variant                     GetOptionDisplayValue(ImportOptions const& option) const;
	bool                        IsImportOptionModified() const;

private:
	VBoxContainer*                           m_optionsContent = nullptr;
	Label*                                   m_header         = nullptr;
	LineEdit*                                m_filter         = nullptr;
	PanelContainer*                          m_objectBar      = nullptr;
	TextureRect*                             m_icon           = nullptr;
	Label*                                   m_empty          = nullptr;
	Label*                                   m_type           = nullptr;
	ScrollContainer*                         m_scroll         = nullptr;
	OptionButton*                            m_importerChoice = nullptr;
	Button*                                  m_reimport       = nullptr;
	VirtualPath                              m_cachedSelectedPath;
	std::vector<Ref<ResourceFormatImporter>> m_matchedImporters;
	int                                      m_selectedImporterIndex = 0;
	std::vector<InspectorProperty*>          m_importOptionProperties;
	std::unordered_map<std::string, Variant> m_importOptions;
	PopupPanel*                              m_changePopup = nullptr;
	Label*                                   m_changeError = nullptr;
	VirtualPath                              m_pendingPath;
	int                                      m_pendingImporter = -1;
	bool                                     m_changing        = false;
	bool                                     m_hasImportConfig = false;
	std::string                              m_configImporterClassName;
	std::unordered_map<std::string, Variant> m_configImportOptions;
};
