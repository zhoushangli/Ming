#include "MingEngine/Editor/UI/ImportPanel.hpp"

#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/FileSystemPanel.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <filesystem>

ImportPanel::ImportPanel() : EditorPanel("Import") {}

ImportPanel::~ImportPanel()
{
	ClearImportOptionProperties();
}

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
			[this, name = option.m_propertyInfo.m_name](Variant const& value)
		{
			m_importOptions[name] = value;
		};

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

	m_cachedSelectedPath     = selectedPath;
	m_matchedImporters       = ResourceImporter::GetMatchedImporters(selectedPath);
	m_selectedImporterIndex  = 0;
	RebuildImportOptionProperties();
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

	m_selectedImporterIndex = std::clamp(
		m_selectedImporterIndex,
		0,
		static_cast<int>(m_matchedImporters.size()) - 1);

	Ref<ResourceFormatImporter> selectedImporter = GetSelectedImporter();
	std::string preview = selectedImporter.IsValid() ? selectedImporter->GetVisibleName() : "";

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
		for (int i = 0; i < static_cast<int>(options.size()) && i < static_cast<int>(m_importOptionProperties.size()); ++i)
		{
			InspectorProperty* property = m_importOptionProperties[i];
			if (property != nullptr)
			{
				property->Render(context, GetOptionDisplayValue(options[i]));
			}
		}
	}

	float const buttonWidth = 120.f;
	float const availableHeight = ImGui::GetContentRegionAvail().y;
	if (availableHeight > ImGui::GetFrameHeightWithSpacing())
	{
		ImGui::Dummy(ImVec2(0.f, availableHeight - ImGui::GetFrameHeightWithSpacing()));
	}

	float const buttonX = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth;
	if (buttonX > ImGui::GetCursorPosX())
	{
		ImGui::SetCursorPosX(buttonX);
	}

	if (ImGui::Button("Reimport", ImVec2(buttonWidth, 0.f)))
	{
		if (!ResourceImporter::Import(selectedPath, GetSelectedImporter(), m_importOptions))
		{
			context.m_editorUI->Warning("Import Failed", selectedPath);
		}
	}

	ImGui::End();
}
