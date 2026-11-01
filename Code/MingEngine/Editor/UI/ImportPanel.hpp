#pragma once

#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class VBoxContainer;
class Label;

class ImportPanel final : public EditorPanel
{
	MCLASS(ImportPanel, EditorPanel);

public:
	ImportPanel();
	~ImportPanel() override;

private:
	void OnInitialize() override;
	void OnRender(EditorUIContext& context) override;
	void Reimport();

	void                        RefreshSelection(VirtualPath const& selectedPath);
	void                        RefreshImportConfigCache(VirtualPath const& selectedPath);
	void                        ApplyCachedImportOptions();
	int                         FindImporterIndexByClassName(std::string const& importerClassName) const;
	void                        RebuildImportOptionProperties();
	void                        ClearImportOptionProperties();
	Ref<ResourceFormatImporter> GetSelectedImporter() const;
	Variant                     GetOptionDisplayValue(ImportOptions const& option) const;
	bool                        IsImportOptionModified() const;

private:
	VBoxContainer* m_optionsContent = nullptr;
	Label* m_header = nullptr;
	OptionButton* m_importerChoice = nullptr;
	Button* m_reimport = nullptr;
	VirtualPath                              m_cachedSelectedPath;
	std::vector<Ref<ResourceFormatImporter>> m_matchedImporters;
	int                                      m_selectedImporterIndex = 0;
	std::vector<InspectorProperty*>          m_importOptionProperties;
	std::unordered_map<std::string, Variant> m_importOptions;
	bool                                     m_hasImportConfig = false;
	std::string                              m_configImporterClassName;
	std::unordered_map<std::string, Variant> m_configImportOptions;
};
