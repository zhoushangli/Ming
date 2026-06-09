#pragma once

#include <string>

struct EditorUIContext;

class UIData
{
public:
	virtual ~UIData() = default;
};

class EditorPanel
{
public:
	explicit EditorPanel(char const* title, bool isOpen = true);
	virtual ~EditorPanel() = default;

	void Open();
	void Open(UIData const& data);
	void Close();
	void Render(EditorUIContext& context);

	bool        IsOpen() const;
	bool*       GetOpenState();
	char const* GetTitle() const;

protected:
	virtual void OnOpen(UIData const& data);
	virtual void OnClose();
	virtual void OnRender(EditorUIContext& context) = 0;

private:
	void ApplyClose();

private:
	std::string m_title;
	bool        m_isOpen       = true;
	bool        m_closePending = false;
};
