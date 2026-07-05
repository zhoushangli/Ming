#include "MingEngine/Editor/UI/ImportPanel.hpp"

#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/FileSystemPanel.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <filesystem>

ImportPanel::ImportPanel() : EditorPanel("Import") {}

ImportPanel::~ImportPanel() { ClearImportOptionProperties(); }

void ImportPanel::ClearImportOptionProperties()
{
	for (InspectorProperty* property : m_importOptionProperties)
	{
		delete property;
	}
	m_importOptionProperties.clear();
}

Ref<ResourceFormatImporter> ImportPanel::GetSelectedImporter() const
{
	if (m_selectedImporterIndex < 0 || m_selectedImporterIndex >= static_cast<int>(m_matchedImporters.size()))
	{
		return Ref<ResourceFormatImporter>();
	}

	return m_matchedImporters[m_selectedImporterIndex];
}

Variant ImportPanel::GetOptionDisplayValue(ImportOptions const& option) const
{
	auto const iter = m_importOptions.find(option.m_propertyInfo.m_name);
	if (iter != m_importOptions.end() && iter->second.GetType() == option.m_propertyInfo.m_type)
	{
		return iter->second;
	}

	return option.m_defaultValue;
}

int ImportPanel::FindImporterIndexByClassName(std::string const& importerClassName) const
{
	for (int i = 0; i < static_cast<int>(m_matchedImporters.size()); ++i)
	{
		Ref<ResourceFormatImporter> importer = m_matchedImporters[i];
		if (importer.IsValid() && importer->GetClassName() == importerClassName)
		{
			return i;
		}
	}

	return -1;
}

void ImportPanel::RefreshImportConfigCache(std::string const& selectedPath)
{
	m_hasImportConfig =
		ResourceImporter::TryReadImportConfig(selectedPath, m_configImporterClassName, m_configImportOptions);
	if (!m_hasImportConfig)
	{
		m_configImporterClassName.clear();
		m_configImportOptions.clear();
	}
}

void ImportPanel::ApplyCachedImportOptions()
{
	m_importOptions.clear();

	Ref<ResourceFormatImporter> importer = GetSelectedImporter();
	if (!m_hasImportConfig || !importer.IsValid() || importer->GetClassName() != m_configImporterClassName)
	{
		return;
	}

	m_importOptions = m_configImportOptions;
}

void ImportPanel::RebuildImportOptionProperties()
{
	ClearImportOptionProperties();
	m_importOptions.clear();

	Ref<ResourceFormatImporter> importer = GetSelectedImporter();
	if (!importer.IsValid())
	{
		return;
	}

	std::vector<ImportOptions> const options = importer->GetImportOptions();
	for (ImportOptions const& option : options)
	{
		std::string labelId = "##ImportOption::" + importer->GetVisibleName() + "::" + option.m_propertyInfo.m_name;
		InspectorProperty::ValueChangedCallback onChanged =
			[this, name = option.m_propertyInfo.m_name](Variant const& value) { m_importOptions[name] = value; };

		InspectorProperty* property =
			InspectorProperty::Create(option.m_propertyInfo, std::move(labelId), std::move(onChanged));
		m_importOptionProperties.push_back(property);
	}
}

void ImportPanel::RefreshSelection(std::string const& selectedPath)
{
	if (m_cachedSelectedPath == selectedPath)
	{
		return;
	}

	m_cachedSelectedPath    = selectedPath;
	m_matchedImporters      = ResourceImporter::GetMatchedImporters(selectedPath);
	m_selectedImporterIndex = 0;

	if (!m_matchedImporters.empty())
	{
		ResourceImporter::EnsureImported(selectedPath);
		RefreshImportConfigCache(selectedPath);

		int const configImporterIndex = FindImporterIndexByClassName(m_configImporterClassName);
		if (configImporterIndex >= 0)
		{
			m_selectedImporterIndex = configImporterIndex;
		}
	}
	else
	{
		m_hasImportConfig = false;
		m_configImporterClassName.clear();
		m_configImportOptions.clear();
	}

	RebuildImportOptionProperties();
	ApplyCachedImportOptions();
}

// 1) Compare current import options with saved config
// 2) Return true if any option differs from the last imported state
bool ImportPanel::IsImportOptionModified() const
{
	Ref<ResourceFormatImporter> importer = GetSelectedImporter();

	// No saved config: modified if user has changed any option from default
	if (!m_hasImportConfig)
	{
		return !m_importOptions.empty();
	}

	// Saved config exists but importer was switched
	if (!importer.IsValid() || importer->GetClassName() != m_configImporterClassName)
	{
		return true;
	}

	// Same importer: compare option values
	if (m_importOptions.size() != m_configImportOptions.size())
	{
		return true;
	}

	for (auto const& [key, value] : m_configImportOptions)
	{
		auto const iter = m_importOptions.find(key);
		if (iter == m_importOptions.end() || iter->second != value)
		{
			return true;
		}
	}

	return false;
}

void ImportPanel::OnRender(EditorUIContext& context)
{
	ImGui::Begin(GetTitle(), GetOpenState());

	if (context.m_editorUI == nullptr)
	{
		ImGui::TextDisabled("No editor context");
		ImGui::End();
		return;
	}

	std::string const& selectedPath = context.m_editorUI->GetPanel<FileSystemPanel>().GetSelectedVirtualPath();
	RefreshSelection(selectedPath);

	if (selectedPath.empty())
	{
		ImGui::TextDisabled("No file selected");
		ImGui::End();
		return;
	}

	std::string const filename = std::filesystem::path(selectedPath).filename().string();
	ImGui::TextUnformatted(filename.c_str());
	ImGui::Separator();

	if (m_matchedImporters.empty())
	{
		ImGui::TextDisabled("No importer available");
		ImGui::End();
		return;
	}

	m_selectedImporterIndex = std::clamp(m_selectedImporterIndex, 0, static_cast<int>(m_matchedImporters.size()) - 1);

	Ref<ResourceFormatImporter> selectedImporter = GetSelectedImporter();
	std::string                 preview          = selectedImporter.IsValid() ? selectedImporter->GetVisibleName() : "";

	ImGui::TextUnformatted("Import As:");
	if (ImGui::BeginCombo("##ImportAs", preview.c_str()))
	{
		for (int i = 0; i < static_cast<int>(m_matchedImporters.size()); ++i)
		{
			Ref<ResourceFormatImporter> importer = m_matchedImporters[i];
			if (!importer.IsValid())
			{
				continue;
			}

			bool const isSelected = i == m_selectedImporterIndex;
			if (ImGui::Selectable(importer->GetVisibleName().c_str(), isSelected))
			{
				if (m_selectedImporterIndex != i)
				{
					m_selectedImporterIndex = i;
					RebuildImportOptionProperties();
					ApplyCachedImportOptions();
				}
			}

			if (isSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	selectedImporter = GetSelectedImporter();
	if (selectedImporter.IsValid())
	{
		std::vector<ImportOptions> const options = selectedImporter->GetImportOptions();
		for (int i = 0; i < static_cast<int>(options.size()) && i < static_cast<int>(m_importOptionProperties.size());
			 ++i)
		{
			InspectorProperty* property = m_importOptionProperties[i];
			if (property != nullptr)
			{
				property->Render(context, GetOptionDisplayValue(options[i]));
			}
		}
	}

	bool const        modified    = IsImportOptionModified();
	std::string const buttonLabel = modified ? "Reimport (*)" : "Reimport";

	float const textWidth       = ImGui::CalcTextSize(buttonLabel.c_str()).x;
	float const buttonWidth     = textWidth + ImGui::GetStyle().FramePadding.x * 2.f;
	float const availableHeight = ImGui::GetContentRegionAvail().y;
	if (availableHeight > ImGui::GetFrameHeightWithSpacing())
	{
		ImGui::Dummy(ImVec2(0.f, availableHeight - ImGui::GetFrameHeightWithSpacing()));
	}

	float const availableWidth = ImGui::GetContentRegionAvail().x;
	float const buttonX        = ImGui::GetCursorPosX() + (availableWidth - buttonWidth) * 0.5f;

	if (buttonX > ImGui::GetCursorPosX())
	{
		ImGui::SetCursorPosX(buttonX);
	}

	if (ImGui::Button(buttonLabel.c_str(), ImVec2(buttonWidth, 0.f)))
	{
		if (!ResourceImporter::Import(selectedPath, GetSelectedImporter(), m_importOptions))
		{
			context.m_editorUI->Warning("Import Failed", selectedPath);
		}
		else
		{
			// Read .import config again to refresh the cache
			RefreshImportConfigCache(selectedPath);
			// Refresh panel ui
			ApplyCachedImportOptions();
			// Update the resource
			ResourceLoader::Reload(selectedPath);
		}
	}

	ImGui::End();
}
