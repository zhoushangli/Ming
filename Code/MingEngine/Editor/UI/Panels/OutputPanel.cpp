#include "MingEngine/Editor/UI/Panels/OutputPanel.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

namespace
{
char const* const logStyles[] = { "EditorLogInfo", "EditorLogWarning", "EditorLogError" };
char const* const logIcons[] = { "Info", "StatusWarning", "StatusError" };

class OutputLogContent final : public Control
{
	MCLASS(OutputLogContent, Control);

public:
	using Position = std::pair<size_t, uint32_t>;

private:
	struct Entry
	{
		String text;
		int level;
		size_t firstLine = 0;
		String countText;
		TextLine count;
	};

	struct Line
	{
		TextLine text;
		size_t entry;
		uint32_t offset;
		float top;
		float height;
	};

public:
	OutputLogContent() { SetMouseFilter(MouseFilter::Ignore); }

	Vector2 GetInternalMinimumSize() const override { return m_extent; }
	bool HasMessages() const { return !m_entries.empty(); }
	bool HasSelection() const { return m_anchor != m_end; }

	void Append(String const& text, int level)
	{
		m_entries.push_back({ text, level });
		LayoutEntry(m_entries.size() - 1);
	}

	void SetLastCount(size_t count)
	{
		Entry& entry = m_entries.back();
		entry.countText = String(U"\u00d7") + std::to_string(count);
		entry.count = Shape(entry.countText);
		MeasureEntry(m_entries.size() - 1);
	}

	void Reset()
	{
		m_entries.clear();
		m_lines.clear();
		m_extent = Vector2::Zero;
		m_anchor = m_end = {};
	}

	void Commit()
	{
		PropagateMinimumSizeChanged();
		QueueRedraw();
	}

	void BeginSelection(Vector2 point)
	{
		m_anchor = m_end = HitTest(point);
		QueueRedraw();
	}

	void ExtendSelection(Vector2 point)
	{
		Position const end = HitTest(point);
		if (end != m_end)
		{
			m_end = end;
			QueueRedraw();
		}
	}

	void SelectAll()
	{
		if (HasMessages())
		{
			m_anchor = {};
			m_end = { m_entries.size() - 1, m_entries.back().text.Length() };
			QueueRedraw();
		}
	}

	String GetSelectedText() const
	{
		if (!HasSelection())
		{
			return {};
		}

		auto const [first, last] = std::minmax(m_anchor, m_end);
		std::u32string text;
		for (size_t i = first.first; i <= last.first; ++i)
		{
			if (i > first.first)
			{
				text.push_back(U'\n');
			}

			String const& entry = m_entries[i].text;
			uint32_t const from = i == first.first ? first.second : 0;
			uint32_t const to = i == last.first ? last.second : entry.Length();
			for (uint32_t offset = from; offset < to; ++offset)
			{
				text.push_back(entry[offset]);
			}
		}

		return String(text.data(), static_cast<uint32_t>(text.size()));
	}

protected:
	void OnThemeChanged() override
	{
		m_font = GetThemeFont("font", "EditorLogInfo");
		m_fontSize = std::max(1, GetThemeFontSize("font_size", "EditorLogInfo"));
		m_gap = static_cast<float>(GetThemeConstant("separation", "EditorLogRow"));
		m_lines.clear();
		m_extent = Vector2::Zero;
		for (size_t i = 0; i < m_entries.size(); ++i)
		{
			Entry& entry = m_entries[i];
			entry.count = Shape(entry.countText);
			LayoutEntry(i);
		}

		Commit();
	}

	void OnNotification(int notification)
	{
		switch (notification)
		{
		case Notification_Draw:
			DrawLog();
			break;
		}
	}

private:
	TextLine Shape(String const& text) const
	{
		if (!m_font.IsValid() || g_engine == nullptr || g_engine->m_textServer == nullptr)
		{
			return {};
		}

		return g_engine->m_textServer->GetTextLine(m_font->GetFontRID(), text, static_cast<float>(m_fontSize));
	}

	float TextStart(Entry const& entry) const
	{
		return m_fontSize + m_gap + (entry.count.m_width > 0.f ? entry.count.m_width + m_gap : 0.f);
	}

	void LayoutEntry(size_t index)
	{
		Entry& entry = m_entries[index];
		entry.firstLine = m_lines.size();
		uint32_t offset = 0;
		while (true)
		{
			uint32_t end = offset;
			while (end < entry.text.Length() && entry.text[end] != U'\n')
			{
				++end;
			}

			TextLine line = Shape(entry.text.Substr(offset, end - offset));
			float const height = std::max(
				static_cast<float>(m_fontSize), std::abs(line.m_config.m_ascent) + std::abs(line.m_config.m_descent));
			m_lines.push_back({ std::move(line), index, offset, m_extent.y, height });
			m_extent.y += height;
			if (end == entry.text.Length())
			{
				break;
			}

			offset = end + 1;
		}

		MeasureEntry(index);
	}

	void MeasureEntry(size_t index)
	{
		Entry const& entry = m_entries[index];
		size_t const end = index + 1 < m_entries.size() ? m_entries[index + 1].firstLine : m_lines.size();
		for (size_t i = entry.firstLine; i < end; ++i)
		{
			m_extent.x = std::max(m_extent.x, TextStart(entry) + m_lines[i].text.m_width);
		}
	}

	auto FindLine(float y) const
	{
		return std::upper_bound(
			m_lines.begin(),
			m_lines.end(),
			y,
			[](float value, Line const& line)
			{
				return value < line.top + line.height;
			});
	}

	static float ColumnX(TextLine const& line, uint32_t column)
	{
		return column < line.m_glyphs.size() ? line.m_glyphs[column].m_posX : line.m_width;
	}

	Position HitTest(Vector2 point) const
	{
		if (m_lines.empty() || point.y < 0.f)
		{
			return {};
		}

		auto const line = FindLine(point.y);
		if (line == m_lines.end())
		{
			return { m_entries.size() - 1, m_entries.back().text.Length() };
		}

		float const x = point.x - TextStart(m_entries[line->entry]);
		uint32_t column = 0;
		while (column < line->text.m_glyphs.size()
			&& x >= (ColumnX(line->text, column) + ColumnX(line->text, column + 1)) * 0.5f)
		{
			++column;
		}

		return { line->entry, line->offset + column };
	}

	void DrawTextLine(TextLine const& line, float x, float y, Color const& color)
	{
		if (!m_font.IsValid() || g_engine == nullptr || g_engine->m_textServer == nullptr)
		{
			return;
		}

		for (TextLineGlyph const& glyph : line.m_glyphs)
		{
			GlyphTexture const* texture =
				g_engine->m_textServer->GetOrCreateGlyphTexture(m_font->GetFontRID(), glyph.m_glyphIndex, m_fontSize);
			if (texture == nullptr || !texture->m_texture.IsValid())
			{
				continue;
			}

			GlyphImage const& bitmap = texture->m_bitmap;
			Rect2 target;
			target.m_position = Vector2(x + glyph.m_posX + bitmap.m_offsetX, y + line.m_config.m_ascent + bitmap.m_offsetY);
			target.m_size = Vector2(static_cast<float>(bitmap.m_width), static_cast<float>(bitmap.m_height));
			Rect2 source;
			source.m_size = target.m_size;
			DrawTextureRectRegion(texture->m_texture, target, source, color);
		}
	}

	void DrawLog()
	{
		Color const dim = GetThemeColor("font_color", "EditorLogTime");
		if (m_lines.empty())
		{
			DrawTextLine(Shape("No messages"), 0.f, 0.f, dim);
			return;
		}

		auto const* scroll = static_cast<ScrollContainer const*>(GetParent());
		if (scroll == nullptr)
		{
			return;
		}

		float const top = scroll->GetScroll();
		float const bottom = top + scroll->GetPageSize().y;
		auto const [first, last] = std::minmax(m_anchor, m_end);
		Color const highlight = GetThemeColor("selection_color", "EditorLogArea");
		for (auto it = FindLine(top); it != m_lines.end() && it->top < bottom; ++it)
		{
			Line const& line = *it;
			Entry const& entry = m_entries[line.entry];
			float const x = TextStart(entry);
			Color const color = GetThemeColor("font_color", logStyles[entry.level]);
			uint32_t const columns = static_cast<uint32_t>(line.text.m_glyphs.size());
			Position const lineEnd(line.entry, line.offset + columns);
			if (first != last && first <= lineEnd && last > Position(line.entry, line.offset))
			{
				uint32_t const from = first.first == line.entry ? std::max(first.second, line.offset) - line.offset : 0;
				uint32_t const to = last < lineEnd ? last.second - line.offset : columns;
				float const start = x + ColumnX(line.text, from);
				float const end = x + ColumnX(line.text, to) + (last > lineEnd ? m_fontSize * 0.35f : 0.f);
				if (end > start)
				{
					DrawRect(AABB2(start, line.top, end, line.top + line.height), highlight);
				}
			}

			if (line.offset == 0)
			{
				Rect2 icon;
				icon.m_position = Vector2(0.f, line.top);
				icon.m_size = Vector2(static_cast<float>(m_fontSize), static_cast<float>(m_fontSize));
				Ref<TextureResource> const texture = GetThemeIcon(logIcons[entry.level], "EditorIcons");
				if (texture.IsValid())
				{
					DrawTextureRect(texture, icon, false, entry.level == 0 ? dim : color);
				}

				DrawTextLine(entry.count, m_fontSize + m_gap, line.top, dim);
			}

			DrawTextLine(line.text, x, line.top, color);
		}
	}

private:
	std::vector<Entry> m_entries;
	std::vector<Line> m_lines;
	Ref<FontResource> m_font;
	int m_fontSize = 14;
	float m_gap = 5.f;
	Vector2 m_extent = Vector2::Zero;
	Position m_anchor;
	Position m_end;
};
}

class OutputLogView final : public ScrollContainer
{
	MCLASS(OutputLogView, ScrollContainer);

public:
	explicit OutputLogView(EditorUI* editorUI) : m_editorUI(editorUI)
	{
		SetFocusMode(FocusMode::Click);
		SetThemeTypeVariation("EditorLogScroll");
		m_content = MemNew<OutputLogContent>();
		AddNode(m_content);
	}

	bool IsSelecting() const { return m_selecting; }

	CursorShape GetCursorShape(Vector2 const& localPosition) const override
	{
		Vector2 const page = GetPageSize();
		if (localPosition.x >= 0.f && localPosition.x < page.x && localPosition.y >= 0.f && localPosition.y < page.y)
		{
			return CursorShape::IBeam;
		}

		return ScrollContainer::GetCursorShape(localPosition);
	}

	void Reset()
	{
		m_selecting = false;
		m_content->Reset();
	}

	void Append(String const& text, int level) { m_content->Append(text, level); }
	void SetLastCount(size_t count) { m_content->SetLastCount(count); }
	void Commit() { m_content->Commit(); }

	bool GuiInput(InputEvent const& event) override
	{
		switch (event.m_type)
		{
		case InputEventType::MouseButton:
		{
			Vector2 const page = GetPageSize();
			bool const inside = event.m_position.x >= 0.f && event.m_position.x < page.x
				&& event.m_position.y >= 0.f && event.m_position.y < page.y;
			if (event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
			{
				if (event.m_pressed && inside)
				{
					m_pointer = event.m_position;
					m_content->BeginSelection(ContentPoint(m_pointer));
					m_selecting = m_content->HasMessages();
					return true;
				}

				if (!event.m_pressed && m_selecting)
				{
					m_content->ExtendSelection(ContentPoint(event.m_position));
					m_selecting = false;
					return true;
				}
			}

			if (event.m_mouseButton == GLFW_MOUSE_BUTTON_RIGHT && event.m_pressed && inside && m_editorUI != nullptr)
			{
				ShowMenu(event.m_position);
				return true;
			}

			break;
		}
		case InputEventType::MouseMotion:
			if (m_selecting)
			{
				m_pointer = event.m_position;
				m_content->ExtendSelection(ContentPoint(m_pointer));
				return true;
			}
			break;
		case InputEventType::MouseWheel:
			if (m_selecting)
			{
				bool const handled = ScrollContainer::GuiInput(event);
				m_content->ExtendSelection(ContentPoint(m_pointer));
				return handled;
			}
			break;
		case InputEventType::Key:
			if (HasFocus() && event.m_pressed
				&& (event.m_modifiers & (GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER | GLFW_MOD_SHIFT))
					== GLFW_MOD_CONTROL)
			{
				switch (event.m_keyCode)
				{
				case GLFW_KEY_C:
					Copy();
					return true;
				case GLFW_KEY_A:
					m_content->SelectAll();
					return true;
				}
			}
			break;
		}

		return ScrollContainer::GuiInput(event);
	}

protected:
	void OnNotification(int notification)
	{
		switch (notification)
		{
		case Notification_Process:
			if (m_selecting && IsVisibleInHierarchy())
			{
				float const overshoot = m_pointer.y - std::clamp(m_pointer.y, 0.f, GetPageSize().y);
				if (overshoot != 0.f)
				{
					float const speed = std::clamp(overshoot * 12.f, -720.f, 720.f);
					SetScroll(GetScroll() + speed * GetSceneTree()->GetDeltaSeconds());
					m_content->ExtendSelection(ContentPoint(m_pointer));
				}
			}
			break;
		case Notification_FocusExit:
		case Notification_GuiInputCancelled:
		case Notification_ExitTree:
			m_selecting = false;
			break;
		case Notification_VisibilityChanged:
			if (!IsVisibleInHierarchy())
			{
				m_selecting = false;
			}
			break;
		case Notification_Draw:
			if (m_content != nullptr && (m_drawScroll != GetScroll() || m_drawPage != GetPageSize()))
			{
				m_drawScroll = GetScroll();
				m_drawPage = GetPageSize();
				m_content->QueueRedraw();
			}
			break;
		}
	}

private:
	Vector2 ContentPoint(Vector2 point) const
	{
		return point + Vector2(GetHorizontalScroll(), GetScroll());
	}

	void Copy() const
	{
		if (m_content->HasSelection() && g_engine != nullptr && g_engine->m_windowSystem != nullptr)
		{
			g_engine->m_windowSystem->SetClipboardText(m_content->GetSelectedText());
		}
	}

	void ShowMenu(Vector2 position)
	{
		ObjectID const id = GetObjectID();
		EditorUI::MenuItem copy = { "Copy", [id]()
		{
			if (auto* view = ObjectDatabase::GetInstance<OutputLogView>(id))
			{
			view->GrabFocus();
				view->Copy();
			}
		}, "Ctrl+C" };
		copy.m_enabled = m_content->HasSelection();
		EditorUI::MenuItem selectAll = { "Select All", [id]()
		{
			if (auto* view = ObjectDatabase::GetInstance<OutputLogView>(id))
			{
			view->GrabFocus();
				view->m_content->SelectAll();
			}
		}, "Ctrl+A" };
		selectAll.m_enabled = m_content->HasMessages();
		m_editorUI->ShowMenu(this, position + GetGlobalPosition(), { copy, selectAll });
	}

private:
	EditorUI* m_editorUI;
	OutputLogContent* m_content = nullptr;
	Vector2 m_pointer = Vector2::Zero;
	Vector2 m_drawPage = Vector2::Zero;
	float m_drawScroll = -1.f;
	bool m_selecting = false;
};

OutputPanel::OutputPanel() : EditorPanel("Output")
{
}

void OutputPanel::OnInitialize()
{
	ClassDatabase::RegisterAbstractClass<OutputLogContent>();
	ClassDatabase::RegisterAbstractClass<OutputLogView>();

	auto* margin = MemNew<MarginContainer>();

	margin->AddThemeChangedCallback(
		[control = margin]()
		{
			control->SetMargins(
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	AddNode(margin);
	m_layout = MemNew<VBoxContainer>();
	m_layout->AddThemeChangedCallback(
		[control = m_layout]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	margin->AddNode(m_layout);
	auto* area = MemNew<PanelContainer>();
	area->SetThemeTypeVariation("EditorLogArea");
	area->SetVerticalSizeFlags(Size_ExpandFill);
	m_layout->AddNode(area);
	m_log = MemNew<OutputLogView>(m_context.m_editorUI);
	area->AddNode(m_log);
	auto* tools = MemNew<HBoxContainer>();
	tools->AddThemeChangedCallback(
		[control = tools]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")) * 0.5f);
		});
	m_layout->AddNode(tools);
	auto* clear = EditorPopupUtils::AddButton(
		tools,
		"",
		[this]()
		{
			ClearMessages();
		});
	clear->SetName("ClearOutput");
	clear->SetIcon(GetThemeIcon("Close", "EditorIcons"));
	clear->SetThemeTypeVariation("EditorPanelAction");
	auto* combine = EditorPopupUtils::AddButton(
		tools,
		"",
		[this]()
		{
			m_combine = !m_combine;
			m_dirty   = true;
		});
	combine->SetName("CombineOutput");
	combine->SetIcon(GetThemeIcon("ItemList", "EditorIcons"));
	combine->SetThemeTypeVariation("EditorPanelAction");
	combine->SetToggleMode(true);
	auto* search = MemNew<LineEdit>();
	search->SetName("SearchOutput");
	search->SetPlaceholder("Filter Messages");
	search->SetMinimumCharacterWidth(1);
	search->SetHorizontalSizeFlags(Size_ExpandFill);
	search->SetIcon(GetThemeIcon("Search", "EditorIcons"));
	search->SetTextChangedCallback(
		[this](String const& text)
		{
			m_searchText = text.ToUtf8();
			std::transform(
				m_searchText.begin(),
				m_searchText.end(),
				m_searchText.begin(),
				[](unsigned char ch)
				{
					return static_cast<char>(std::tolower(ch));
				});
			m_dirty = true;
		});
	tools->AddNode(search);
	Button* clearSearch = EditorPopupUtils::AddButton(
		tools,
		"",
		[this, search]()
		{
			search->SetText("");
			m_searchText.clear();
			m_dirty = true;
			search->GrabFocus();
		});
	clearSearch->SetName("ClearOutputSearch");
	clearSearch->SetIcon(GetThemeIcon("Close", "EditorIcons"));
	clearSearch->SetThemeTypeVariation("EditorPanelAction");
	char const* icons[]  = { "Info", "StatusWarning", "StatusError" };
	char const* colors[] = { "tn", "twarn", "terr" };
	for (int i = 0; i < 3; ++i)
	{
		auto* button = EditorPopupUtils::AddButton(
			tools,
			"0",
			[this, i]()
			{
				m_filters[i] = !m_filters[i];
				m_dirty      = true;
			});
		button->SetName(icons[i]);
		button->SetThemeTypeVariation("EditorOutputFilter");
		button->SetIcon(GetThemeIcon(icons[i], "EditorIcons"));
		button->AddThemeColorOverride("font_color", GetThemeColor(colors[i], "EditorIcons"));
		button->AddThemeColorOverride("font_pressed_color", GetThemeColor(colors[i], "EditorIcons"));
		button->AddThemeColorOverride("font_hover_pressed_color", GetThemeColor(colors[i], "EditorIcons"));
		button->SetToggleMode(true);
		m_typeFilters[i] = button;
	}
}

void OutputPanel::AddMessage(Message message)
{
	String const& original = message.m_text;
	uint32_t end = original.Length();
	while (end > 0 && (original[end - 1] == U'\n' || original[end - 1] == U'\r'))
	{
		--end;
	}

	std::u32string text;
	text.reserve(end);
	for (uint32_t i = 0; i < end; ++i)
	{
		if (original[i] == U'\r')
		{
			text.push_back(U'\n');
			if (i + 1 < end && original[i + 1] == U'\n')
			{
				++i;
			}
		}
		else
		{
			text.push_back(original[i]);
		}
	}

	message.m_text = String(text.data(), static_cast<uint32_t>(text.size()));
	++m_messageCounts[static_cast<int>(message.m_level)];
	m_messages.push_back(std::move(message));
}

void OutputPanel::ClearMessages()
{
	m_messages.clear();
	for (size_t& count : m_messageCounts)
	{
		count = 0;
	}

	m_dirty = true;
}

void OutputPanel::SetCollapsed(bool collapsed)
{
	m_collapsed = collapsed;
	static_cast<Control*>(m_layout->GetParent())->SetVisible(!collapsed);
}

void OutputPanel::UpdateMessages()
{
	bool const atBottom = m_log->GetScroll() >= m_log->GetContentSize().y - m_log->GetPageSize().y - 1.f;
	m_restoreScroll = atBottom && !m_log->IsSelecting();
	m_savedScroll = m_log->GetScroll();

	if (m_dirty)
	{
		m_log->Reset();
		m_processed = 0;
		m_repetitions = 0;
		m_dirty = false;
	}

	for (; m_processed < m_messages.size(); ++m_processed)
	{
		Message const& message = m_messages[m_processed];
		int const level = static_cast<int>(message.m_level);
		if (!m_filters[level])
		{
			continue;
		}

		if (!m_searchText.empty())
		{
			std::string lower = message.m_text.ToUtf8();
			std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch)
			{
				return static_cast<char>(std::tolower(ch));
			});
			if (lower.find(m_searchText) == std::string::npos)
			{
				continue;
			}
		}

		if (m_combine && m_processed > 0 && m_messages[m_processed - 1].m_level == message.m_level
			&& m_messages[m_processed - 1].m_text == message.m_text)
		{
			m_log->SetLastCount(++m_repetitions);
		}
		else
		{
			m_log->Append(message.m_text, level);
			m_repetitions = 1;
		}
	}

	m_log->Commit();
}

void OutputPanel::OnRender(EditorUIContext&)
{
	if (m_restoreScroll)
	{
		if (!m_log->IsSelecting() && m_log->GetScroll() == m_savedScroll)
		{
			m_log->SetScroll(m_log->GetContentSize().y);
		}

		m_restoreScroll = false;
	}
	for (int i = 0; i < 3; ++i)
	{
		m_typeFilters[i]->SetText(std::to_string(m_messageCounts[i]));
		m_typeFilters[i]->SetPressedNoSignal(m_filters[i]);
	}
	auto* tabs = dynamic_cast<TabContainer*>(GetParent());
	if (tabs != nullptr)
	{
		Button* output = tabs->GetTabButton(tabs->GetPageIndex(this));
		output->SetText("Output " + std::to_string(m_messages.size()));
	}

	if (m_dirty || m_processed != m_messages.size())
	{
		UpdateMessages();
	}
}
