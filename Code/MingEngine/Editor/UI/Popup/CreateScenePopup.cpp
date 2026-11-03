#include "MingEngine/Editor/UI/Popup/CreateScenePopup.hpp"

#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <filesystem>
#include <utility>

namespace
{
std::string ToLower(std::string text)
{
	std::transform(
		text.begin(),
		text.end(),
		text.begin(),
		[](unsigned char character)
		{
			return static_cast<char>(std::tolower(character));
		});
	return text;
}

bool IsReservedWindowsName(std::string const& name)
{
	std::string const stem = ToLower(std::filesystem::u8path(name).stem().u8string());
	if (stem == "con" || stem == "prn" || stem == "aux" || stem == "nul")
	{
		return true;
	}

	return stem.size() == 4 && (stem.compare(0, 3, "com") == 0 || stem.compare(0, 3, "lpt") == 0) && stem[3] >= '1'
		   && stem[3] <= '9';
}

} // namespace

void CreateScenePopup::Open(VirtualPath const& parentVirtualPath)
{
	m_parentVirtualPath   = parentVirtualPath;
	m_selectedVirtualPath = {};
	m_error.clear();
	m_overwrite      = {};
	m_sceneName[0]   = '\0';
	m_rootName[0]    = '\0';
	m_focusNameInput = true;
	m_openRequested  = true;
}

void CreateScenePopup::Render(EditorUIContext& context)
{
	m_context = context;
	if (m_popup == nullptr && !m_openRequested)
	{
		return;
	}

	if (m_popup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_popup                = EditorPopupUtils::Create(context.m_rootControl, "Create Scene", content);
		EditorPopupUtils::AddLabel(content, "Name:");
		m_nameEdit = MemNew<LineEdit>();
		m_nameEdit->SetTextChangedCallback(
			[this](String const&)
			{
				m_overwrite = {};
				m_confirm->SetText("Create");
			});
		content->AddNode(m_nameEdit);
		m_nameEdit->SetEditingEndedCallback(
			[this](String const&, LineEdit::EditEndReason reason)
			{
				if (reason == LineEdit::EditEndReason::Submitted)
				{
					Confirm();
				}
			});
		EditorPopupUtils::AddLabel(content, "Root name (empty uses scene name):");
		m_rootEdit = MemNew<LineEdit>();
		content->AddNode(m_rootEdit);
		m_rootEdit->SetEditingEndedCallback(
			[this](String const&, LineEdit::EditEndReason reason)
			{
				if (reason == LineEdit::EditEndReason::Submitted)
				{
					Confirm();
				}
			});
		m_errorLabel  = EditorPopupUtils::AddLabel(content, "", "EditorErrorLabel");
		auto* buttons = EditorPopupUtils::AddActions(content);
		m_confirm     = EditorPopupUtils::AddButton(
			buttons,
			"Create",
			[this]()
			{
				Confirm();
			});
		EditorPopupUtils::AddButton(
			buttons,
			"Cancel",
			[this]()
			{
				m_popup->Close();
			});
	}

	if (m_openRequested)
	{
		m_openRequested = false;
		m_confirm->SetText("Create");
		m_nameEdit->SetText(m_sceneName);
		m_rootEdit->SetText(m_rootName);
		m_errorLabel->SetText("");
		EditorPopupUtils::Open(
			m_popup,
			context.m_rootControl,
			Vector2(
				static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(context.m_rootControl->GetThemeConstant("create_popup_height", "Editor"))));
		m_nameEdit->GrabFocus();
		m_nameEdit->SelectAll();
	}

	if (m_popup->IsVisible())
	{
		EditorPopupUtils::Layout(
			m_popup,
			Vector2(
				static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(context.m_rootControl->GetThemeConstant("create_popup_height", "Editor"))));
	}
}

bool CreateScenePopup::ConsumeSelectedVirtualPath(VirtualPath& outVirtualPath)
{
	if (!m_selectedVirtualPath.IsValid())
	{
		return false;
	}

	outVirtualPath        = std::move(m_selectedVirtualPath);
	m_selectedVirtualPath = {};
	return true;
}

bool CreateScenePopup::ValidateEntryName(char const* name, std::string& outError) const
{
	std::string const value = name != nullptr ? name : "";
	if (value.empty())
	{
		outError = "Name cannot be empty.";
		return false;
	}

	if (value == "." || value == ".." || value.front() == ' ' || value.back() == ' ' || value.back() == '.')
	{
		outError = "Name cannot be '.', '..', or start/end with a space.";
		return false;
	}

	if (value.find_first_of("<>:\"/\\|?*") != std::string::npos || IsReservedWindowsName(value))
	{
		outError = "Name contains invalid characters or is reserved by Windows.";
		return false;
	}

	outError.clear();
	return true;
}

void CreateScenePopup::Confirm()
{
	std::string const sceneName = m_nameEdit->GetText().ToUtf8();
	std::string const rootName  = m_rootEdit->GetText().IsEmpty() ? sceneName : m_rootEdit->GetText().ToUtf8();
	if (!ValidateEntryName(sceneName.c_str(), m_error) || !ValidateEntryName(rootName.c_str(), m_error))
	{
		m_errorLabel->SetText(m_error);
		return;
	}

	VirtualPath const path = m_parentVirtualPath.Join(sceneName + ".tscn");
	if (std::filesystem::u8path(sceneName).has_extension())
	{
		m_error = "Scene name must not include an extension.";
	}
	else if (m_context.m_fileSystem == nullptr)
	{
		m_error = "FileSystem unavailable.";
	}
	else if (m_context.m_fileSystem->Exists(path) && m_overwrite != path)
	{
		m_overwrite = path;
		m_error     = "This scene already exists. Press Overwrite to replace it.";
		m_confirm->SetText("Overwrite");
	}
	else if (EditorNode::Get() != nullptr)
	{
		m_popup->Close();
		EditorNode::Get()->RequestCreateScene(
			path,
			rootName,
			[this, path]()
			{
				m_selectedVirtualPath = path;
			});
		return;
	}

	m_errorLabel->SetText(m_error);
}
