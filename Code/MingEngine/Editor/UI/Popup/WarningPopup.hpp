#pragma once

#include <string>
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"

struct EditorUIContext;

class WarningPopup
{
public:
	void Open(std::string const& title, std::string const& message);
	void Render(EditorUIContext& context);

private:
	PopupPanel* m_popup = nullptr;
	Label* m_heading = nullptr;
	Label* m_text = nullptr;
	bool        m_openRequested = false;
	std::string m_title;
	std::string m_message;
};
