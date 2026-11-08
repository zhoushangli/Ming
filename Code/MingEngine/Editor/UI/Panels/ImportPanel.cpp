#include "MingEngine/Editor/UI/Panels/ImportPanel.hpp"

#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Panels/FileSystemPanel.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"

#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"

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
			property->SetRevertValue(option.m_defaultValue);
			m_optionsContent->AddNode(property->GetControl());
			m_importOptionProperties.push_back(property);
		}
	}
}

void ImportPanel::RefreshSelection(VirtualPath const& selectedPath, bool discard)
{
	if (m_cachedSelectedPath == selectedPath)
	{
		return;
	}

	if (!discard && (m_changing || IsImportOptionModified()))
	{
		if (!m_changing)
		{
			RequestSwitch(selectedPath);
		}
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

// 1) Compare current import options with saved config
// 2) Return true if any option differs from the last imported state
bool ImportPanel::IsImportOptionModified() const
{
	Ref<ResourceFormatImporter> importer = GetSelectedImporter();
	if (!importer.IsValid())
	{
		return false;
	}

	if (m_hasImportConfig && importer->GetClassName() != m_configImporterClassName)
	{
		return true;
	}

	for (ImportOptions const& option : importer->GetImportOptions())
	{
		auto          saved    = m_configImportOptions.find(option.m_propertyInfo.m_name);
		Variant const baseline = saved != m_configImportOptions.end() ? saved->second : option.m_defaultValue;
		if (GetOptionDisplayValue(option) != baseline)
		{
			return true;
		}
	}
	return false;
}

void ImportPanel::RequestSwitch(VirtualPath const& path, int importer)
{
	if (m_changePopup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_changePopup          = EditorPopupUtils::Create(m_context.m_rootControl, "Unsaved Import Options", content);
		EditorPopupUtils::AddLabel(content, "Reimport before changing the selection?");
		m_changeError = EditorPopupUtils::AddLabel(content, "", "EditorErrorLabel");
		auto* actions = EditorPopupUtils::AddActions(content);
		EditorPopupUtils::AddButton(
			actions,
			"Reimport",
			[this]()
			{
				ResolveSwitch(SwitchChoice::Reimport);
			});
		EditorPopupUtils::AddButton(
			actions,
			"Discard",
			[this]()
			{
				ResolveSwitch(SwitchChoice::Discard);
			});
		EditorPopupUtils::AddButton(
			actions,
			"Cancel",
			[this]()
			{
				ResolveSwitch(SwitchChoice::Cancel);
			});
		m_changePopup->SetClosedCallback(
			[this]()
			{
				if (m_changing)
				{
					m_changing = false;
					m_context.m_editorUI->GetPanel<FileSystemPanel>().SelectPath(m_cachedSelectedPath);
				}
			});
	}
	m_pendingPath     = path;
	m_pendingImporter = importer;
	m_changing        = true;
	m_changeError->SetText("");
	EditorPopupUtils::Open(
		m_changePopup,
		m_context.m_rootControl,
		Vector2(
			static_cast<float>(GetThemeConstant("project_popup_width", "Editor")),
			static_cast<float>(GetThemeConstant("create_popup_height", "Editor"))));
}

void ImportPanel::ResolveSwitch(SwitchChoice choice)
{
	if (choice == SwitchChoice::Reimport && !Reimport())
	{
		return;
	}

	m_changing = false;
	m_changePopup->Close();
	if (choice == SwitchChoice::Cancel)
	{
		m_context.m_editorUI->GetPanel<FileSystemPanel>().SelectPath(m_cachedSelectedPath);
		return;
	}

	if (m_pendingImporter >= 0)
	{
		m_selectedImporterIndex = m_pendingImporter;
		RebuildImportOptionProperties();
		ApplyCachedImportOptions();
	}
	else
	{
		RefreshSelection(m_pendingPath, true);
	}
}

void ImportPanel::OnInitialize()
{
	auto* layout = MemNew<VBoxContainer>();
	layout->AddThemeChangedCallback(
		[control = layout]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	auto* margin = MemNew<MarginContainer>();
	margin->AddThemeChangedCallback(
		[control = margin]()
		{
			control->SetMargins(
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	AddNode(margin);
	margin->AddNode(layout);
	layout->UpdateSize(GetSize());
	m_filter = MemNew<LineEdit>();
	m_filter->SetPlaceholder("Filter properties");
	m_filter->SetMinimumCharacterWidth(1);
	m_filter->SetIcon(GetThemeIcon("Search", "EditorIcons"));
	layout->AddNode(m_filter);
	m_empty = EditorPopupUtils::AddLabel(layout, "Select a source file in FileSystem.", "EditorStatusLabel");
	m_empty->SetAutowrapMode(AutowrapMode::WordSmart);
	m_objectBar = MemNew<PanelContainer>();
	m_objectBar->SetThemeTypeVariation("EditorObjectBar");
	layout->AddNode(m_objectBar);
	auto* row = MemNew<HBoxContainer>();
	row->AddThemeChangedCallback(
		[control = row]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("status_spacing", "Editor")) * 0.5f);
		});
	m_objectBar->AddNode(row);
	m_icon = MemNew<TextureRect>();
	m_icon->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
	m_icon->SetStretchMode(TextureRect::StretchMode::KeepAspectCentered);
	m_icon->AddThemeChangedCallback(
		[control = m_icon]()
		{
			control->SetCustomMinimumSize(Vector2(
				static_cast<float>(control->GetThemeConstant("icon_size", "Button")),
				static_cast<float>(control->GetThemeConstant("icon_size", "Button"))));
		});
	row->AddNode(m_icon);
	m_header = EditorPopupUtils::AddLabel(row, "");
	m_header->SetHorizontalSizeFlags(Size_ExpandFill);
	m_header->SetClipText(true);
	m_header->SetVerticalAlignment(VerticalAlignment::Center);
	m_type           = EditorPopupUtils::AddLabel(layout, "IMPORT", "EditorPropertyLabel");
	m_importerChoice = MemNew<OptionButton>();
	layout->AddNode(m_importerChoice);
	m_importerChoice->SetValueChangedCallback(
		[this](int index)
		{
			if (IsImportOptionModified())
			{
				RequestSwitch(m_cachedSelectedPath, index);
				m_importerChoice->SetValueNoSignal(m_selectedImporterIndex);
				return;
			}
			m_selectedImporterIndex = index;
			RebuildImportOptionProperties();
			ApplyCachedImportOptions();
		});
	auto* scroll = MemNew<ScrollContainer>();
	m_scroll     = scroll;
	scroll->SetVerticalSizeFlags(Size_ExpandFill);
	layout->AddNode(scroll);
	m_optionsContent = MemNew<VBoxContainer>();
	scroll->AddNode(m_optionsContent);
	m_reimport = EditorPopupUtils::AddButton(
		layout,
		"Reimport",
		[this]()
		{
			Reimport();
		});
	m_reimport->SetHorizontalSizeFlags(Size_ShrinkEnd);
}

void ImportPanel::OnRender(EditorUIContext& context)
{
	VirtualPath const previous = m_cachedSelectedPath;
	RefreshSelection(context.m_editorUI->GetPanel<FileSystemPanel>().GetSelectedVirtualPath());
	VirtualPath const selected = m_cachedSelectedPath;
	bool const        changed  = selected != previous;
	bool const        valid    = GetSelectedImporter().IsValid();
	m_header->SetText(valid ? selected.GetFileName() : "");
	m_objectBar->SetVisible(valid);
	m_empty->SetVisible(!valid);
	m_type->SetVisible(valid);
	m_scroll->SetVisible(valid);
	m_reimport->SetVisible(valid);
	if (valid)
	{
		std::string const extension = selected.GetExtension();
		char const*       icon      = extension == ".png" || extension == ".jpg" ? "Texture2D" : "Resource";
		m_icon->SetTexture(GetThemeIcon(icon, "EditorIcons"));
		m_icon->SetTint(EditorIcons::GetIconColor(icon, this));
		m_type->SetText("IMPORT AS " + GetSelectedImporter()->GetVisibleName());
	}
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
			std::string name  = m_importOptionProperties[i]->GetDisplayName();
			std::string query = m_filter->GetText().ToUtf8();
			std::transform(
				name.begin(),
				name.end(),
				name.begin(),
				[](unsigned char c)
				{
					return static_cast<char>(std::tolower(c));
				});
			std::transform(
				query.begin(),
				query.end(),
				query.begin(),
				[](unsigned char c)
				{
					return static_cast<char>(std::tolower(c));
				});
			m_importOptionProperties[i]->GetControl()->SetVisible(
				query.empty() || name.find(query) != std::string::npos);
		}
	}
}

bool ImportPanel::Reimport()
{
	if (!ResourceImporter::Import(m_cachedSelectedPath, GetSelectedImporter(), m_importOptions))
	{
		std::string const error = "Could not reimport: " + m_cachedSelectedPath.GetString();
		ERR_PRINT(error);
		if (m_changing)
		{
			m_changeError->SetText(error);
		}
		else
		{
			m_context.m_editorUI->Warning("Import Failed", error);
		}
		return false;
	}

	RefreshImportConfigCache(m_cachedSelectedPath);
	ApplyCachedImportOptions();
	ResourceLoader::Reload(m_cachedSelectedPath);
	INFO_PRINT("Reimported: " + m_cachedSelectedPath.GetString());
	return true;
}
