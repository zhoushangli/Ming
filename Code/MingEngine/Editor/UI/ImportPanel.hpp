#pragma once

#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class ImportPanel final : public EditorPanel
{
public:
	ImportPanel();
	~ImportPanel() override;

private:
	void OnRender(EditorUIContext& context) override;

	void RefreshSelection(std::string const& selectedPath);
	void RebuildImportOptionProperties();
	void ClearImportOptionProperties();
	Ref<ResourceFormatImporter> GetSelectedImporter() const;
	Variant GetOptionDisplayValue(ImportOptions const& option) const;

private:
	std::string                                      m_cachedSelectedPath;
	std::vector<Ref<ResourceFormatImporter>>         m_matchedImporters;
	int                                              m_selectedImporterIndex = 0;
	std::vector<InspectorProperty*>                  m_importOptionProperties;
	std::unordered_map<std::string, Variant>         m_importOptions;
};
