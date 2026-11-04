#pragma once

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Engine/File/VirtualPath.hpp"
#include <functional>

class PopupPanel;
class LineEdit;
class OptionButton;
class Button;
class Label;

class CreateScriptPopup
{
public:
	void Open(VirtualPath const& directory, std::string const& base, std::function<void(VirtualPath const&)> created);
	void Render(EditorUIContext& context);
	bool Create(VirtualPath const& path, std::string const& base, bool useTemplate, std::string& error);

private:
	void Confirm();
	bool Validate(VirtualPath const& path, std::string const& base, std::string& error) const;
	void UpdateValidation();

private:
	EditorUIContext m_context;
	PopupPanel* m_popup = nullptr;
	LineEdit* m_path = nullptr;
	OptionButton* m_base = nullptr;
	Button* m_template = nullptr;
	Button* m_confirm = nullptr;
	Label* m_error = nullptr;
	VirtualPath m_directory;
	std::string m_initialBase;
	std::function<void(VirtualPath const&)> m_created;
	bool m_openRequested = false;
};
