#include "MingEngine/Editor/UI/ImportPanel.hpp"

#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/FileSystemPanel.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"

#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"

#include <algorithm>
#include <filesystem>

ImportPanel::ImportPanel() : EditorPanel("Import")
{
}

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

void ImportPanel::RefreshImportConfigCache(VirtualPath const& selectedPath)
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
			[this, name = option.m_propertyInfo.m_name](Variant const& value)
		{
			m_importOptions[name] = value;
		};

		InspectorProperty* property =
			InspectorProperty::Create(option.m_propertyInfo, std::move(labelId), std::move(onChanged));
		if (property != nullptr)
		{
			m_optionsContent->AddNode(property->GetControl());
			m_importOptionProperties.push_back(property);
		}
	}
}

void ImportPanel::RefreshSelection(VirtualPath const& selectedPath)
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

void ImportPanel::OnInitialize()
{
	auto* layout = MemNew<VBoxContainer>();
	layout->SetSeparation(10.f);
	AddNode(layout);
	layout->UpdateSize(GetSize());
	m_header = EditorPopupUtils::AddLabel(layout, "Select an importable file");
	m_header->SetClipText(true);
	m_importerChoice = MemNew<OptionButton>();
	layout->AddNode(m_importerChoice);
	m_importerChoice->SetValueChangedCallback([this](int index)
	{
		m_selectedImporterIndex = index;
		RebuildImportOptionProperties();
		ApplyCachedImportOptions();
	});
	auto* scroll = MemNew<ScrollContainer>();
	scroll->SetVerticalSizeFlags(Size_ExpandFill);
	layout->AddNode(scroll);
	m_optionsContent = MemNew<VBoxContainer>();
	scroll->AddNode(m_optionsContent);
	m_reimport = EditorPopupUtils::AddButton(layout, "Reimport", [this]() { Reimport(); });
	m_reimport->SetThemeTypeVariation("EditorPrimaryButton");
}

void ImportPanel::OnRender(EditorUIContext& context)
{
	VirtualPath const selected = context.m_editorUI->GetPanel<FileSystemPanel>().GetSelectedVirtualPath();
	bool const changed = selected != m_cachedSelectedPath;
	RefreshSelection(selected);
	bool const valid = GetSelectedImporter().IsValid();
	m_header->SetText(valid ? selected.GetFileName() : "Select an importable file");
	if (changed)
	{
		std::string hint;
		for (int i = 0; i < static_cast<int>(m_matchedImporters.size()); ++i)
		{
			if (!hint.empty())
			{
				hint += ',';
			}
			hint += m_matchedImporters[i]->GetVisibleName() + ":" + std::to_string(i);
		}
		m_importerChoice->SetOptions(hint);
	}

	m_importerChoice->SetValueNoSignal(m_selectedImporterIndex);
	m_importerChoice->SetVisible(valid);
	m_reimport->SetDisabled(!valid);
	m_reimport->SetText(IsImportOptionModified() ? "Reimport (*)" : "Reimport");
	if (valid)
	{
		auto const options = GetSelectedImporter()->GetImportOptions();
		for (size_t i = 0; i < options.size() && i < m_importOptionProperties.size(); ++i)
		{
			m_importOptionProperties[i]->UpdateValue(GetOptionDisplayValue(options[i]));
		}
	}
}

void ImportPanel::Reimport()
{
	if (!ResourceImporter::Import(m_cachedSelectedPath, GetSelectedImporter(), m_importOptions))
	{
		m_context.m_editorUI->Warning("Import Failed", m_cachedSelectedPath.GetString());
		return;
	}

	RefreshImportConfigCache(m_cachedSelectedPath);
	ApplyCachedImportOptions();
	ResourceLoader::Reload(m_cachedSelectedPath);
}
