#include "MingEngine/Editor/UI/Popup/SceneFilePopup.hpp"

#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/IO/ResourceImporter.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"

#include <algorithm>
#include <filesystem>

void SceneFilePopup::Open(
	Mode                                    mode,
	VirtualPath const&                      initial,
	std::function<bool(VirtualPath const&)> accepted,
	std::function<void()>                   cancelled)
{
	m_mode      = mode;
	m_initial   = initial;
	m_accepted  = std::move(accepted);
	m_cancelled = std::move(cancelled);
	m_requested = true;
	m_overwrite = {};
}

void SceneFilePopup::Render(EditorUIContext& context)
{
	m_context = context;
	if (m_requested && m_popup == nullptr)
	{
		m_popup = EditorPopupUtils::Create(context.m_rootControl, "Select File", m_content);
		m_popup->SetClosedCallback(
			[this]()
			{
				if (!m_accepting && m_cancelled)
				{
					auto cancelled = std::move(m_cancelled);
					cancelled();
				}
			});
		auto* navigation = MemNew<HBoxContainer>();
		m_content->AddNode(navigation);
		EditorPopupUtils::AddButton(
			navigation,
			"Up",
			[this]()
			{
				Navigate(m_directory.GetParent());
			});
		m_path = MemNew<LineEdit>();
		m_path->SetName("SceneDirectory");
		m_path->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		m_path->SetMinimumCharacterWidth(1);
		navigation->AddNode(m_path);
		m_path->SetEditingEndedCallback(
			[this](String const& text, LineEdit::EditEndReason reason)
			{
				VirtualPath path;
				if (reason == LineEdit::EditEndReason::Submitted && VirtualPath::TryParse(text.ToUtf8(), path))
				{
					Navigate(path);
				}
			});
		m_tree = MemNew<Tree>();
		m_tree->SetVerticalSizeFlags(Control::Size_ExpandFill);
		m_tree->SetClipText(true);
		m_content->AddNode(m_tree);
		m_tree->SetSelectedCallback(
			[this](uint64_t key)
			{
				auto found = m_entries.find(key);
				if (found != m_entries.end() && IsSupportedFile(found->second))
				{
					m_name->SetText(found->second.GetFileName());
				}
			});
		m_tree->SetActivatedCallback(
			[this](uint64_t key)
			{
				auto found = m_entries.find(key);
				if (found != m_entries.end())
				{
					std::filesystem::path physical;
					std::error_code       error;
					if (m_context.m_fileSystem->TryGetPhysicalPath(found->second, physical)
						&& std::filesystem::is_directory(physical, error))
					{
						Navigate(found->second);
					}
					else
					{
						Confirm();
					}
				}
			});
		m_fileLabel = EditorPopupUtils::AddLabel(m_content, "");
		m_name = MemNew<LineEdit>();
		m_name->SetName("SceneFilename");
		m_name->SetMinimumCharacterWidth(1);
		m_content->AddNode(m_name);
		m_name->SetTextChangedCallback(
			[this](String const&)
			{
				m_overwrite = {};
				m_confirm->SetText(GetConfirmText());
			});
		m_name->SetEditingEndedCallback(
			[this](String const&, LineEdit::EditEndReason reason)
			{
				if (reason == LineEdit::EditEndReason::Submitted)
				{
					Confirm();
				}
			});
		m_error       = EditorPopupUtils::AddLabel(m_content, "", "EditorErrorLabel");
		auto* actions = EditorPopupUtils::AddActions(m_content);
		m_confirm     = EditorPopupUtils::AddButton(
			actions,
			"",
			[this]()
			{
				Confirm();
			});
		EditorPopupUtils::AddButton(
			actions,
			"Cancel",
			[this]()
			{
				m_popup->Close();
			});
	}

	if (m_requested)
	{
		m_requested = false;
		m_error->SetText("");
		m_confirm->SetText(GetConfirmText());
		bool const initialFile = IsSupportedFile(m_initial);
		m_fileLabel->SetText(m_mode == Mode::ResourceSave ? "Text resource (.tres)" : m_mode == Mode::ResourceOpen ? "Resource file" : m_mode == Mode::Autoload ? "Script (.cs) or scene (.tscn)" : "Scene file (.tscn)");
		m_name->SetText(initialFile ? m_initial.GetFileName() : "");
		VirtualPath const directory = initialFile ? m_initial.GetParent() : m_initial;
		std::filesystem::path physical;
		std::error_code error;
		if (m_context.m_fileSystem->TryGetPhysicalPath(directory, physical) && std::filesystem::is_directory(physical, error))
		{
			Navigate(directory);
		}
		else
		{
			Navigate(VirtualPath("res://"));
		}
		EditorPopupUtils::Open(
			m_popup,
			context.m_rootControl,
			Vector2(
				static_cast<float>(m_content->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(m_content->GetThemeConstant("project_popup_height", "Editor")) * 1.5f));
		m_name->GrabFocus();
		m_name->SelectAll();
	}

	if (m_popup != nullptr && m_popup->IsVisible())
	{
		EditorPopupUtils::Layout(
			m_popup,
			Vector2(
				static_cast<float>(m_content->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(m_content->GetThemeConstant("project_popup_height", "Editor")) * 1.5f));
	}
}

bool SceneFilePopup::IsSupportedFile(VirtualPath const& path) const
{
	if (m_mode == Mode::ResourceSave)
	{
		return path.HasExtension(".tres");
	}

	if (m_mode == Mode::ResourceOpen)
	{
		return ResourceLoader::CanLoad(path) || ResourceImporter::CanImport(path);
	}

	return path.HasExtension(".tscn") || (m_mode == Mode::Autoload && path.HasExtension(".cs"));
}

char const* SceneFilePopup::GetConfirmText() const
{
	switch (m_mode)
	{
	case Mode::ResourceSave:
	case Mode::Save:      return "Save";
	case Mode::ResourceOpen: return "Load Resource";
	case Mode::Open:      return "Open";
	case Mode::Autoload:  return "Select Autoload";
	case Mode::MainScene: return "Select Main Scene";
	}

	return "";
}

void SceneFilePopup::Navigate(VirtualPath const& directory)
{
	std::filesystem::path physical;
	std::error_code       error;
	if (!directory.IsValid() || !m_context.m_fileSystem->TryGetPhysicalPath(directory, physical)
		|| !std::filesystem::is_directory(physical, error))
	{
		m_error->SetText("Select an existing project directory.");
		return;
	}

	m_directory = directory;
	m_overwrite = {};
	m_confirm->SetText(GetConfirmText());
	m_path->SetText(directory.GetString());
	Refresh();
}

void SceneFilePopup::Refresh()
{
	m_entries.clear();
	std::vector<Tree::Item> items;
	std::filesystem::path   physical;
	m_context.m_fileSystem->TryGetPhysicalPath(m_directory, physical);
	std::error_code                               error;
	std::vector<std::filesystem::directory_entry> entries;
	for (std::filesystem::directory_iterator iterator(physical, error), end; !error && iterator != end;
		 iterator.increment(error))
	{
		entries.push_back(*iterator);
	}
	std::sort(
		entries.begin(),
		entries.end(),
		[](auto const& left, auto const& right)
		{
			return left.path().filename() < right.path().filename();
		});
	for (auto const& entry : entries)
	{
		bool const        directory = entry.is_directory(error);
		VirtualPath const path      = m_directory.Join(entry.path().filename().u8string());
		if (!path.IsValid() || (!directory && !IsSupportedFile(path))
			|| entry.path().filename().u8string().front() == '.')
		{
			continue;
		}

		uint64_t const key = m_entries.size() + 1;
		m_entries[key]     = path;
		items.push_back(
			{ key,
			  0,
			  path.GetFileName(),
			  true,
			  false,
			  m_content->GetThemeIcon(directory ? "Folder" : path.HasExtension(".cs") ? "CSharpScript" : "PackedScene", "EditorIcons") });
	}
	m_tree->SetItems(std::move(items));
}

bool SceneFilePopup::Confirm()
{
	std::string name = m_name->GetText().ToUtf8();
	if ((m_mode == Mode::Save || m_mode == Mode::ResourceSave) && std::filesystem::u8path(name).extension().empty())
	{
		name += m_mode == Mode::ResourceSave ? ".tres" : ".tscn";
	}
	VirtualPath const path = m_directory.Join(name);
	if (name.empty() || name.find_first_of("/\\<>:\"|?*") != std::string::npos || !path.IsValid()
		|| !IsSupportedFile(path))
	{
		m_error->SetText(m_mode == Mode::Autoload ? "Enter a script (.cs) or scene (.tscn) filename."
			: m_mode == Mode::ResourceSave ? "Enter a resource filename ending in .tres." : m_mode == Mode::ResourceOpen ? "Select a supported resource file." : "Enter a scene filename ending in .tscn.");
		return false;
	}

	if (m_mode == Mode::Autoload && path.HasExtension(".cs"))
	{
		std::filesystem::path physical;
		std::error_code error;
		if (!m_context.m_fileSystem->TryGetPhysicalPath(path, physical) || !std::filesystem::is_regular_file(physical, error))
		{
			m_error->SetText("The selected C# script does not exist.");
			return false;
		}
	}
	else if (m_mode == Mode::ResourceOpen)
	{
		if (!ResourceLoader::Load(path).IsValid())
		{
			m_error->SetText("This file is not a valid resource.");
			return false;
		}
	}
	else if (m_mode != Mode::Save && m_mode != Mode::ResourceSave)
	{
		Ref<PackedScene> scene(ResourceLoader::LoadUncached(path));
		if (!scene.IsValid())
		{
			m_error->SetText("This file is not a valid scene.");
			return false;
		}
	}
	else if (m_context.m_fileSystem->Exists(path) && m_overwrite != path)
	{
		m_overwrite = path;
		m_error->SetText("This file already exists. Press Overwrite to replace it.");
		m_confirm->SetText("Overwrite");
		return false;
	}

	if (!m_accepted || !m_accepted(path))
	{
		m_error->SetText("The operation failed. The current scene has been retained.");
		return false;
	}

	m_accepting = true;
	m_popup->Close();
	m_accepting = false;
	m_cancelled = {};
	return true;
}
