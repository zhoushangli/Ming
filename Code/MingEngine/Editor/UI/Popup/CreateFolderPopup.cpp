#include "MingEngine/Editor/UI/Popup/CreateFolderPopup.hpp"

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstring>
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
	std::string const stem = ToLower(std::filesystem::path(name).stem().string());
	if (stem == "con" || stem == "prn" || stem == "aux" || stem == "nul")
	{
		return true;
	}

	return stem.size() == 4 && (stem.compare(0, 3, "com") == 0 || stem.compare(0, 3, "lpt") == 0) && stem[3] >= '1'
		   && stem[3] <= '9';
}

} // namespace

void CreateFolderPopup::Open(VirtualPath const& parentVirtualPath)
{
	m_parentVirtualPath   = parentVirtualPath;
	m_selectedVirtualPath = {};
	m_error.clear();
	strcpy_s(m_name, "New Folder");
	m_focusNameInput = true;
	m_openRequested  = true;
}

void CreateFolderPopup::Render(EditorUIContext& context)
{
	m_context = context;
	if (m_popup == nullptr && !m_openRequested)
	{
		return;
	}

	if (m_popup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_popup = EditorPopupUtils::Create(context.m_rootControl, "Create Folder", content);
		EditorPopupUtils::AddLabel(content, "Name:");
		m_nameEdit = MemNew<LineEdit>();
		content->AddNode(m_nameEdit);
		m_nameEdit->SetEditingEndedCallback([this](String const&, LineEdit::EditEndReason reason)
		{
			if (reason == LineEdit::EditEndReason::Submitted)
			{
				Confirm();
			}
		});
		m_errorLabel = EditorPopupUtils::AddLabel(content, "", "EditorErrorLabel");
		auto* buttons = MemNew<HBoxContainer>();
		content->AddNode(buttons);
		EditorPopupUtils::AddButton(buttons, "Create", [this]() { Confirm(); });
		EditorPopupUtils::AddButton(buttons, "Cancel", [this]() { m_popup->Close(); });
	}

	if (m_openRequested)
	{
		m_openRequested = false;
		m_nameEdit->SetText(m_name);
		m_errorLabel->SetText("");
		EditorPopupUtils::Open(m_popup, context.m_rootControl, Vector2(static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_width", "Editor")), static_cast<float>(context.m_rootControl->GetThemeConstant("create_popup_height", "Editor"))));
		m_nameEdit->GrabFocus();
		m_nameEdit->SelectAll();
	}

	if (m_popup->IsVisible())
	{
		EditorPopupUtils::Layout(m_popup, Vector2(static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_width", "Editor")), static_cast<float>(context.m_rootControl->GetThemeConstant("create_popup_height", "Editor"))));
	}
}

bool CreateFolderPopup::ConsumeSelectedVirtualPath(VirtualPath& outVirtualPath)
{
	if (!m_selectedVirtualPath.IsValid())
	{
		return false;
	}

	outVirtualPath        = std::move(m_selectedVirtualPath);
	m_selectedVirtualPath = {};
	return true;
}

bool CreateFolderPopup::ValidateName(EditorUIContext const& context, std::string& outError) const
{
	std::string const value = m_name;
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

	if (context.m_fileSystem == nullptr)
	{
		outError = "FileSystem is not available.";
		return false;
	}

	if (context.m_fileSystem->Exists(m_parentVirtualPath.Join(value)))
	{
		outError = "An entry with this name already exists.";
		return false;
	}

	outError.clear();
	return true;
}

void CreateFolderPopup::Confirm()
{
	strncpy_s(m_name, m_nameEdit->GetText().ToUtf8().c_str(), sizeof(m_name) - 1);
	if (!ValidateName(m_context, m_error))
	{
		m_errorLabel->SetText(m_error);
		return;
	}

	if (m_context.m_fileSystem->CreateFolder(m_parentVirtualPath, m_name, m_selectedVirtualPath, m_error))
	{
		m_context.m_fileSystem->ScanResourceTree();
		m_popup->Close();
	}

	m_errorLabel->SetText(m_error);
}
