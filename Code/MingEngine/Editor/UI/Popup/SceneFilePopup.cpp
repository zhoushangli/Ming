#include "MingEngine/Editor/UI/Popup/SceneFilePopup.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"

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
		m_popup = EditorPopupUtils::Create(context.m_rootControl, "Scene File", m_content);
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
		auto* scroll = MemNew<ScrollContainer>();
		scroll->SetVerticalSizeFlags(Control::Size_ExpandFill);
		m_content->AddNode(scroll);
		m_tree = MemNew<Tree>();
		m_tree->SetClipText(true);
		scroll->AddNode(m_tree);
		m_tree->SetSelectedCallback(
			[this](uint64_t key)
			{
				auto found = m_entries.find(key);
				if (found != m_entries.end() && found->second.HasExtension(".tscn"))
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
		EditorPopupUtils::AddLabel(m_content, "Scene file (.tscn)");
		m_name = MemNew<LineEdit>();
		m_name->SetName("SceneFilename");
		m_name->SetMinimumCharacterWidth(1);
		m_content->AddNode(m_name);
		m_name->SetTextChangedCallback(
			[this](String const&)
			{
				m_overwrite = {};
				m_confirm->SetText(m_mode == Mode::Save ? "Save" : m_mode == Mode::Open ? "Open" : "Select Main Scene");
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
		m_confirm->SetText(m_mode == Mode::Save ? "Save" : m_mode == Mode::Open ? "Open" : "Select Main Scene");
		m_name->SetText(m_initial.HasExtension(".tscn") ? m_initial.GetFileName() : "");
		Navigate(
			m_initial.HasExtension(".tscn") ? m_initial.GetParent()
			: m_initial.IsValid()           ? m_initial
											: VirtualPath("res://"));
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
	m_confirm->SetText(m_mode == Mode::Save ? "Save" : m_mode == Mode::Open ? "Open" : "Select Main Scene");
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
		if (!path.IsValid() || (!directory && !path.HasExtension(".tscn"))
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
			  m_content->GetThemeIcon(directory ? "Folder" : "PackedScene", "EditorIcons") });
	}
	m_tree->SetItems(std::move(items));
}

bool SceneFilePopup::Confirm()
{
	std::string name = m_name->GetText().ToUtf8();
	if (m_mode == Mode::Save && std::filesystem::u8path(name).extension().empty())
	{
		name += ".tscn";
	}
	VirtualPath const path = m_directory.Join(name);
	if (name.empty() || name.find_first_of("/\\<>:\"|?*") != std::string::npos || !path.IsValid()
		|| !path.HasExtension(".tscn"))
	{
		m_error->SetText("Enter a scene filename ending in .tscn.");
		return false;
	}

	if (m_mode != Mode::Save)
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
		m_error->SetText("This scene already exists. Press Overwrite to replace it.");
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
