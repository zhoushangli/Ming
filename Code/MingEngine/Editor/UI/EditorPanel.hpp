#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"

#include <string>

class UIData
{
public:
	virtual ~UIData() = default;
};

class EditorPanel : public Control
{
	MCLASS(EditorPanel, Control);

private:
	struct ThemeCache
	{
		Ref<ThemeStyleBox> panel;
	};

public:
	explicit EditorPanel(char const* title, bool isOpen = true);

	void Initialize(EditorUIContext const& context);
	void Open();
	void Open(UIData const& data);
	void Close();
	void Render(EditorUIContext& context);

	bool        IsOpen() const { return m_isOpen; }
	char const* GetTitle() const { return m_title.c_str(); }
	Vector2     GetInternalMinimumSize() const override;

protected:
	static void  BindMethods();
	virtual void OnInitialize() {}
	virtual void OnOpen(UIData const& data);
	virtual void OnClose();
	virtual void OnRender(EditorUIContext& context) = 0;
	void         OnNotification(int notification);

protected:
	EditorUIContext m_context;

private:
	ThemeCache  m_themeCache;
	std::string m_title;
	bool        m_isOpen      = true;
	bool        m_initialized = false;
};
