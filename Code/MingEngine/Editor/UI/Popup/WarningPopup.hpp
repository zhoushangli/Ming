#pragma once

#include <string>

struct EditorUIContext;

class WarningPopup
{
public:
	void Open(std::string const& title, std::string const& message);
	void Render(EditorUIContext& context);

private:
	bool        m_openRequested = false;
	std::string m_title;
	std::string m_message;
};
