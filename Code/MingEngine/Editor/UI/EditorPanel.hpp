#pragma once

#include <string>

class EditorPanel
{
public:
	explicit EditorPanel(char const* title);

	bool        IsOpen() const;
	void        SetOpen(bool isOpen);
	bool*       GetOpenState();
	char const* GetTitle() const;

private:
	std::string m_title;
	bool        m_isOpen = true;
};
