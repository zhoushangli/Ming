#include "MingEngine/Editor/UI/Popup/CreateScriptPopup.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include <algorithm>
#include <sstream>
#include <set>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void CreateScriptPopup::Open(VirtualPath const& directory, std::string const& base, std::function<void(VirtualPath const&)> created)
{
	m_directory = directory;
	m_initialBase = base;
	m_created = std::move(created);
	m_openRequested = true;
}

void CreateScriptPopup::Render(EditorUIContext& context)
{
	m_context = context;
	if (!m_openRequested)
	{
		if (m_popup != nullptr && m_popup->IsVisible())
		{
			EditorPopupUtils::Layout(m_popup, Vector2(static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_width", "Editor")), 0.f));
		}

		return;
	}

	m_openRequested = false;
	if (m_popup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_popup = EditorPopupUtils::Create(context.m_rootControl, "Create Script", content);
		auto addRow = [content](char const* name)
		{
			auto* row = MemNew<HBoxContainer>();
			content->AddNode(row);
			auto* label = EditorPopupUtils::AddLabel(row, name);
			label->AddThemeChangedCallback([label]()
			{
				label->SetCustomMinimumSize(Vector2(static_cast<float>(label->GetThemeConstant("property_label_width", "Editor")), 0.f));
			});
			label->SetVerticalAlignment(VerticalAlignment::Center);
			return row;
		};
		EditorPopupUtils::AddLabel(addRow("Language:"), "C#");
		m_base = MemNew<OptionButton>();
		m_base->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		addRow("Inherits:")->AddNode(m_base);
		m_template = MemNew<Button>();
		m_template->SetToggleMode(true);
		m_template->SetText("Default callbacks");
		addRow("Template:")->AddNode(m_template);
		m_path = MemNew<LineEdit>();
		m_path->SetMinimumCharacterWidth(1);
		m_path->SetTextChangedCallback([this](String const&) { UpdateValidation(); });
		m_base->SetValueChangedCallback([this](int) { UpdateValidation(); });
		m_path->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		addRow("Path:")->AddNode(m_path);
		m_path->SetEditingEndedCallback([this](String const&, LineEdit::EditEndReason reason)
		{
			if (reason == LineEdit::EditEndReason::Submitted)
			{
				Confirm();
			}
		});
		m_error = EditorPopupUtils::AddLabel(content, "", "EditorErrorLabel");
		m_error->SetAutowrapMode(AutowrapMode::WordSmart);
		auto* actions = EditorPopupUtils::AddActions(content);
		m_confirm = EditorPopupUtils::AddButton(actions, "Create", [this]() { Confirm(); });
		m_confirm->SetThemeTypeVariation("EditorPrimaryButton");
		EditorPopupUtils::AddButton(actions, "Cancel", [this]() { m_popup->Close(); });
	}

	std::istringstream types(g_engine->m_scriptSystem->GetScriptBaseTypes());
	std::string type;
	std::string options;
	int selected = 0;
	int index = 0;
	while (std::getline(types, type, ','))
	{
		if (!options.empty())
		{
			options += ',';
		}

		options += type + ":" + std::to_string(index);
		if (type == m_initialBase)
		{
			selected = index;
		}

		++index;
	}

	m_base->SetOptions(options);
	m_base->SetValueNoSignal(selected);
	m_confirm->SetDisabled(options.empty());
	m_template->SetPressedNoSignal(true);
	m_path->SetText(m_directory.GetString() + (m_directory.IsRoot() ? "" : "/") + "NewScript.cs");
	m_error->SetText(options.empty() ? "C# bindings are unavailable. See Output." : "");
	EditorPopupUtils::Open(m_popup, context.m_rootControl,
		Vector2(static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_width", "Editor")), 0.f));
	m_path->GrabFocus();
	m_path->SelectAll();
	UpdateValidation();
}

bool CreateScriptPopup::Validate(VirtualPath const& path, std::string const& base, std::string& error) const
{
	error.clear();
	std::filesystem::path physical;
	if (!path.HasExtension(".cs") || !g_engine->m_fileSystem->TryGetPhysicalPath(path, physical))
	{
		error = "Choose a .cs path inside this project.";
		return false;
	}

	std::string const name = physical.stem().u8string();
	static std::set<std::string> const keywords = {
		"abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char", "checked", "class", "const",
		"continue", "decimal", "default", "delegate", "do", "double", "else", "enum", "event", "explicit", "extern",
		"false", "finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit", "in", "int", "interface",
		"internal", "is", "lock", "long", "namespace", "new", "null", "object", "operator", "out", "override",
		"params", "private", "protected", "public", "readonly", "ref", "return", "sbyte", "sealed", "short", "sizeof",
		"stackalloc", "static", "string", "struct", "switch", "this", "throw", "true", "try", "typeof", "uint",
		"ulong", "unchecked", "unsafe", "ushort", "using", "virtual", "void", "volatile", "while" };
	auto letter = [](unsigned char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; };
	if (name.empty() || !letter(name.front()) || keywords.count(name) != 0
		|| !std::all_of(name.begin(), name.end(), [letter](unsigned char c) { return letter(c) || (c >= '0' && c <= '9'); }))
	{
		error = "Use a C# class name containing letters, digits or underscores, starting with a letter or underscore.";
		return false;
	}

	std::string const types = "," + g_engine->m_scriptSystem->GetScriptBaseTypes() + ",";
	if (types.find("," + base + ",") == std::string::npos)
	{
		error = "Select an available Node base type.";
		return false;
	}

	std::error_code code;
	if (std::filesystem::exists(physical, code) || !std::filesystem::is_directory(physical.parent_path(), code))
	{
		error = "Choose an existing folder and a filename that is not already used.";
		return false;
	}

	for (auto const& component : std::filesystem::u8path(path.GetString().substr(6)).parent_path())
	{
		if (component == ".ming" || component == "bin" || component == "obj")
		{
			error = "Scripts must be outside generated build directories.";
			return false;
		}
	}

	return true;
}

void CreateScriptPopup::UpdateValidation()
{
	if (m_path == nullptr || m_confirm == nullptr)
	{
		return;
	}

	std::string base;
	for (auto const& item : m_base->GetOptions())
	{
		if (item.m_value == m_base->GetValue())
		{
			base = item.m_name.ToUtf8();
		}
	}

	VirtualPath path;
	std::string error;
	bool const valid = VirtualPath::TryParse(m_path->GetText().ToUtf8(), path) && Validate(path, base, error);
	m_confirm->SetDisabled(!valid);
	m_error->SetText(valid ? "" : error.empty() ? "Enter a valid res:// script path." : error);
}

bool CreateScriptPopup::Create(VirtualPath const& path, std::string const& base, bool useTemplate, std::string& error)
{
	if (!Validate(path, base, error))
	{
		return false;
	}

	std::filesystem::path physical;
	g_engine->m_fileSystem->TryGetPhysicalPath(path, physical);
	std::string const name = physical.stem().u8string();
	if (!g_engine->m_scriptSystem->EnsureProjectSolution())
	{
		error = "Could not prepare the net9.0 project. See Output.";
		return false;
	}

	std::string text = "using Ming;\n\npublic partial class " + name + " : global::Ming." + base + "\n{\n";
	if (useTemplate)
	{
		text += "    public override void OnReady()\n    {\n    }\n\n    public override void OnProcess(float deltaSeconds)\n    {\n    }\n";
	}

	text += "}\n";
	HANDLE file = CreateFileW(physical.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE)
	{
		error = "Could not create script. Choose an existing folder and a filename that is not already used.";
		return false;
	}

	DWORD written = 0;
	bool const success = WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) && written == text.size();
	CloseHandle(file);
	if (!success)
	{
		DeleteFileW(physical.c_str());
		error = "Could not write the script file.";
		return false;
	}

	g_engine->m_fileSystem->ScanResourceTree();
	return true;
}

void CreateScriptPopup::Confirm()
{
	VirtualPath path;
	std::string error;
	std::string base;
	for (auto const& item : m_base->GetOptions())
	{
		if (item.m_value == m_base->GetValue())
		{
			base = item.m_name.ToUtf8();
		}
	}

	if (!VirtualPath::TryParse(m_path->GetText().ToUtf8(), path) || !Create(path, base, m_template->IsPressed(), error))
	{
		m_error->SetText(error.empty() ? "Enter a valid res:// script path." : error);
		return;
	}

	auto callback = std::move(m_created);
	m_popup->Close();
	if (callback)
	{
		callback(path);
	}
}
