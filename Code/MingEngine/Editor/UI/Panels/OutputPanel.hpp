#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

#include <vector>

class VBoxContainer;
class Button;
class OutputLogView;

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
	void UpdateMessages();

private:
	std::vector<Message> m_messages;
	VBoxContainer*       m_layout         = nullptr;
	OutputLogView*       m_log            = nullptr;
	Button*              m_typeFilters[3] = {};
	size_t               m_messageCounts[3] = {};
	bool                 m_filters[3]     = { true, true, true };
	bool                 m_combine        = false;
	bool                 m_restoreScroll  = false;
	float                m_savedScroll    = 0.f;
	size_t               m_processed      = 0;
	size_t               m_repetitions    = 0;
	std::string          m_searchText;
	bool                 m_collapsed = false;
	bool                 m_dirty     = true;
};
