#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

#include <vector>

class VBoxContainer;
class OptionButton;
class Button;
class ScrollContainer;

class OutputPanel final : public EditorPanel
{
	MCLASS(OutputPanel, EditorPanel);

public:
	enum class Level
	{
		Info,
		Warning,
		Error
	};

	struct Message
	{
		String m_time;
		Level  m_level = Level::Info;
		String m_text;
	};

public:
	OutputPanel();
	void   AddMessage(Message message);
	void   ClearMessages();
	void   SetCollapsed(bool collapsed);
	bool   IsCollapsed() const { return m_collapsed; }
	size_t GetMessageCount() const { return m_messages.size(); }

private:
	void OnInitialize() override;
	void OnRender(EditorUIContext& context) override;
	void RebuildMessages();

private:
	std::vector<Message> m_messages;
	VBoxContainer*       m_layout         = nullptr;
	VBoxContainer*       m_lines          = nullptr;
	Button*              m_typeFilters[3] = {};
	bool                 m_filters[3]     = { true, true, true };
	bool                 m_combine        = false;
	bool                 m_follow         = false;
	bool                 m_restoreScroll  = false;
	float                m_savedScroll    = 0.f;
	ScrollContainer*     m_scroll         = nullptr;
	std::string          m_searchText;
	bool                 m_collapsed = false;
	bool                 m_dirty     = true;
};
