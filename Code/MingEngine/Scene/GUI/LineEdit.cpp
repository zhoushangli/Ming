#include "MingEngine/Scene/GUI/LineEdit.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include <algorithm>
#include <cmath>
#include <string>

LineEdit::LineEdit()
{
	SetFocusMode(FocusMode::All);
	SetMouseFilter(MouseFilter::Stop);
}

LineEdit::~LineEdit()
{
	if (EditorNode* editor = EditorNode::Get())
	{
		editor->GetUndoRedo().ClearHistory(GetObjectID());
	}
}

void LineEdit::BindMethods()
{
	ClassDatabase::BindMethod(
		"RestoreEditState",
		&LineEdit::RestoreEditState,
		{ "text", "cursorColumn", "scrollOffset" });
}

bool LineEdit::IsTextCodepoint(char32_t codepoint)
{
	return codepoint >= 0x20 && !(codepoint >= 0x7f && codepoint <= 0x9f)
		   && !(codepoint >= 0xd800 && codepoint <= 0xdfff) && codepoint <= 0x10ffff && codepoint != 0x2028
		   && codepoint != 0x2029;
}

String LineEdit::FilterText(String const& text)
{
	std::u32string filtered;
	filtered.reserve(text.Length());
	for (uint32_t i = 0; i < text.Length(); ++i)
	{
		if (IsTextCodepoint(text[i]))
		{
			filtered.push_back(text[i]);
		}
	}

	return String(filtered.data(), static_cast<uint32_t>(filtered.size()));
}

void LineEdit::SetText(String const& text)
{
	String const   filtered = FilterText(text);
	ObjectID const id       = GetObjectID();
	EndEditing();
	LineEdit* edit = ObjectDatabase::GetInstance<LineEdit>(id);
	if (edit == nullptr || edit->m_editing)
	{
		return;
	}

	if (EditorNode* editor = EditorNode::Get())
	{
		editor->GetUndoRedo().ClearHistory(GetObjectID());
	}

	m_text            = filtered;
	m_cursorColumn    = 0;
	m_selectionAnchor = 0;
	m_scrollOffset    = 0.f;
	m_selecting       = false;
	UpdateTextLayout();
}

void LineEdit::SetPlaceholder(String const& text)
{
	m_placeholder = FilterText(text);
	UpdateTextLayout();
}

void LineEdit::SetFont(Ref<FontResource> const& font)
{
	if (m_font == font)
	{
		return;
	}

	m_font = font;
	UpdateTextLayout();
	PropagateMinimumSizeChanged();
}

void LineEdit::SetFontSize(int fontSize)
{
	if (fontSize <= 0 || m_fontSize == fontSize)
	{
		return;
	}

	m_fontSize = fontSize;
	UpdateTextLayout();
	PropagateMinimumSizeChanged();
}

void LineEdit::SetPadding(Vector2 const& padding)
{
	m_padding = Vector2(std::max(0.f, padding.x), std::max(0.f, padding.y));
	EnsureCursorVisible();
	PropagateMinimumSizeChanged();
	QueueRedraw();
}

void LineEdit::SetEditable(bool editable)
{
	if (m_editable == editable)
	{
		return;
	}

	m_editable = editable;
	QueueRedraw();
	if (!editable)
	{
		EndEditing();
	}
}

void LineEdit::SetDisabled(bool disabled)
{
	if (m_disabled == disabled)
	{
		return;
	}

	m_disabled = disabled;
	QueueRedraw();
	if (disabled)
	{
		m_selecting = false;
		Deselect();
		ObjectID const id = GetObjectID();
		EndEditing();
		if (LineEdit* edit = ObjectDatabase::GetInstance<LineEdit>(id);
			edit != nullptr && edit->m_disabled && edit->GetViewport() != nullptr)
		{
			edit->GetViewport()->CancelGuiInteraction(edit);
		}
	}
}

bool LineEdit::BeginEditing()
{
	if (!m_editable || m_disabled || m_isPreDeleting || IsQueuedForDeletion() || !IsVisibleInHierarchy()
		|| (GetViewport() != nullptr && !GetViewport()->IsWindowFocused()))
	{
		return false;
	}

	if (m_editing)
	{
		return true;
	}

	m_editing               = true;
	uint64_t const version  = ++m_editVersion;
	ObjectID const id       = GetObjectID();
	auto           callback = m_editingStartedCallback;
	String const   value    = m_text;
	if (callback)
	{
		callback(value);
	}

	LineEdit* edit = ObjectDatabase::GetInstance<LineEdit>(id);
	return edit != nullptr && edit->m_editing && edit->m_editVersion == version && edit->m_editable && !edit->m_disabled
		   && !edit->m_isPreDeleting && !edit->IsQueuedForDeletion() && edit->IsVisibleInHierarchy();
}

void LineEdit::EndEditing(EditEndReason reason)
{
	if (!m_editing)
	{
		return;
	}

	m_editing = false;
	++m_editVersion;
	m_selecting = false;
	QueueRedraw();

	auto         callback = m_editingEndedCallback;
	String const value    = m_text;
	if (callback)
	{
		callback(value, reason);
	}
}

Vector2 LineEdit::GetInternalMinimumSize() const
{
	float width  = 0.f;
	float height = static_cast<float>(m_fontSize);
	if (m_font.IsValid() && m_font->GetFontRID().IsValid() && g_engine != nullptr && g_engine->m_textServer != nullptr)
	{
		TextLine const minimum =
			g_engine->m_textServer->GetTextLine(m_font->GetFontRID(), std::string(m_minimumCharacterWidth, 'W'), static_cast<float>(m_fontSize));
		width  = minimum.m_width;
		height = std::abs(minimum.m_config.m_ascent) + std::abs(minimum.m_config.m_descent);
	}

	return Vector2(std::max(1.f, width), height) + m_padding * 2.f;
}

void LineEdit::SetMinimumCharacterWidth(int width)
{
	m_minimumCharacterWidth = std::max(1, width);
	PropagateMinimumSizeChanged();
}

void LineEdit::UpdateTextLayout()
{
	m_line            = {};
	m_placeholderLine = {};
	if (m_font.IsValid() && m_font->GetFontRID().IsValid() && g_engine != nullptr && g_engine->m_textServer != nullptr)
	{
		TextServer* server = g_engine->m_textServer;
		m_line             = server->GetTextLine(m_font->GetFontRID(), m_text, static_cast<float>(m_fontSize));
		m_placeholderLine  = server->GetTextLine(m_font->GetFontRID(), m_placeholder, static_cast<float>(m_fontSize));
	}

	EnsureCursorVisible();
	QueueRedraw();
}

AABB2 LineEdit::GetContentRect() const
{
	Vector2 const size = GetSize();
	Vector2 const inset(std::min(size.x * 0.5f, m_padding.x), std::min(size.y * 0.5f, m_padding.y));
	return AABB2(inset, size - inset);
}

float LineEdit::GetColumnPosition(int column) const
{
	if (column >= 0 && column < static_cast<int>(m_line.m_glyphs.size()))
	{
		return m_line.m_glyphs[column].m_posX;
	}

	return m_line.m_width;
}

void LineEdit::EnsureCursorVisible()
{
	float const width    = std::max(0.f, GetContentRect().GetWidth() - 1.f);
	float const position = GetColumnPosition(m_cursorColumn);
	if (position < m_scrollOffset)
	{
		m_scrollOffset = position;
	}
	else if (position > m_scrollOffset + width)
	{
		m_scrollOffset = position - width;
	}

	m_scrollOffset = std::clamp(m_scrollOffset, 0.f, std::max(0.f, m_line.m_width - width));
}

void LineEdit::MoveCursor(int column, bool select)
{
	m_cursorColumn = std::clamp(column, 0, static_cast<int>(m_text.Length()));
	if (!select)
	{
		m_selectionAnchor = m_cursorColumn;
	}

	EnsureCursorVisible();
	QueueRedraw();
}

void LineEdit::SetCursorColumn(int column)
{
	MoveCursor(column, false);
}

int LineEdit::GetSelectionFromColumn() const
{
	return std::min(m_selectionAnchor, m_cursorColumn);
}

int LineEdit::GetSelectionToColumn() const
{
	return std::max(m_selectionAnchor, m_cursorColumn);
}

String LineEdit::GetSelectedText() const
{
	return m_text.Substr(GetSelectionFromColumn(), GetSelectionToColumn() - GetSelectionFromColumn());
}

void LineEdit::Select(int from, int to)
{
	m_selectionAnchor = std::clamp(from, 0, static_cast<int>(m_text.Length()));
	MoveCursor(to, true);
}

void LineEdit::SelectAll()
{
	Select(0, static_cast<int>(m_text.Length()));
}

void LineEdit::Deselect()
{
	m_selectionAnchor = m_cursorColumn;
	QueueRedraw();
}

void LineEdit::ReplaceSelection(String const& text)
{
	String const previousText   = m_text;
	int const    previousCursor = m_cursorColumn;
	float const  previousScroll = m_scrollOffset;

	int const    from    = GetSelectionFromColumn();
	int const    to      = GetSelectionToColumn();
	String const result  = m_text.Substr(0, from) + text + m_text.Substr(to);
	bool const   changed = result != m_text;
	if (changed && !BeginEditing())
	{
		return;
	}

	m_text            = result;
	m_cursorColumn    = from + static_cast<int>(text.Length());
	m_selectionAnchor = m_cursorColumn;
	UpdateTextLayout();

	if (changed)
	{
		if (EditorNode* editor = EditorNode::Get())
		{
			EditorUndoRedo& history = editor->GetUndoRedo();
			if (history.CreateAction("Edit Text", GetObjectID()))
			{
				history.AddDoMethod(this, "RestoreEditState", { m_text, m_cursorColumn, m_scrollOffset });
				history.AddUndoMethod(this, "RestoreEditState", { previousText, previousCursor, previousScroll });
				history.CommitAction(false);
			}
		}

		EmitTextChanged();
	}
}

void LineEdit::RestoreEditState(String const& text, int cursorColumn, float scrollOffset)
{
	bool const changed = m_text != text;
	if (changed && !BeginEditing())
	{
		return;
	}

	m_text            = text;
	m_cursorColumn    = std::clamp(cursorColumn, 0, static_cast<int>(m_text.Length()));
	m_selectionAnchor = m_cursorColumn;
	m_scrollOffset    = scrollOffset;
	m_selecting       = false;
	UpdateTextLayout();

	if (changed)
	{
		EmitTextChanged();
	}
}

void LineEdit::EmitTextChanged()
{
	auto         callback = m_textChangedCallback;
	String const value    = m_text;
	if (callback)
	{
		callback(value);
	}
}

void LineEdit::CopyText()
{
	if (!m_disabled && HasSelection() && g_engine != nullptr && g_engine->m_windowSystem != nullptr)
	{
		g_engine->m_windowSystem->SetClipboardText(GetSelectedText());
	}
}

void LineEdit::CutText()
{
	if (!m_disabled && m_editable && HasSelection() && g_engine != nullptr && g_engine->m_windowSystem != nullptr
		&& g_engine->m_windowSystem->SetClipboardText(GetSelectedText()))
	{
		ReplaceSelection(String());
	}
}

void LineEdit::PasteText()
{
	if (m_disabled || !m_editable || g_engine == nullptr || g_engine->m_windowSystem == nullptr)
	{
		return;
	}

	String const text = FilterText(g_engine->m_windowSystem->GetClipboardText());
	if (!text.IsEmpty())
	{
		ReplaceSelection(text);
	}
}

void LineEdit::Undo()
{
	if (EditorNode* editor = EditorNode::Get(); editor != nullptr && m_editable && !m_disabled)
	{
		editor->GetUndoRedo().Undo(GetObjectID());
	}
}

void LineEdit::Redo()
{
	if (EditorNode* editor = EditorNode::Get(); editor != nullptr && m_editable && !m_disabled)
	{
		editor->GetUndoRedo().Redo(GetObjectID());
	}
}

bool LineEdit::HasUndo() const
{
	EditorNode* editor = EditorNode::Get();
	return editor != nullptr && editor->GetUndoRedo().HasUndo(GetObjectID());
}

bool LineEdit::HasRedo() const
{
	EditorNode* editor = EditorNode::Get();
	return editor != nullptr && editor->GetUndoRedo().HasRedo(GetObjectID());
}

int LineEdit::GetColumnAtPosition(float localX) const
{
	float const position = localX - GetContentRect().m_mins.x + m_scrollOffset;
	int         column   = 0;
	float       distance = std::abs(position - GetColumnPosition(0));
	for (int i = 1; i <= static_cast<int>(m_line.m_glyphs.size()); ++i)
	{
		float const nextDistance = std::abs(position - GetColumnPosition(i));
		if (nextDistance < distance)
		{
			distance = nextDistance;
			column   = i;
		}
	}

	return column;
}

bool LineEdit::GuiInput(InputEvent const& event)
{
	if (m_disabled)
	{
		return false;
	}

	if (event.m_type == InputEventType::MouseButton && event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (event.m_pressed)
		{
			MoveCursor(GetColumnAtPosition(event.m_position.x), (event.m_modifiers & GLFW_MOD_SHIFT) != 0);
			m_selecting = true;
		}
		else
		{
			m_selecting = false;
		}

		return true;
	}

	if (event.m_type == InputEventType::MouseMotion && m_selecting)
	{
		MoveCursor(GetColumnAtPosition(event.m_position.x), true);
		return true;
	}

	if (!HasFocus())
	{
		return false;
	}

	if (event.m_type == InputEventType::Text)
	{
		if (m_editable && IsTextCodepoint(event.m_codepoint))
		{
			char32_t const codepoint = static_cast<char32_t>(event.m_codepoint);
			ReplaceSelection(String(&codepoint, 1));
		}

		return true;
	}

	if (event.m_type != InputEventType::Key || !event.m_pressed)
	{
		return false;
	}

	bool const shift   = (event.m_modifiers & GLFW_MOD_SHIFT) != 0;
	bool const command = (event.m_modifiers & (GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER)) != 0;
	if (!command
		&& (event.m_keyCode == GLFW_KEY_ENTER || event.m_keyCode == GLFW_KEY_KP_ENTER
			|| event.m_keyCode == GLFW_KEY_ESCAPE))
	{
		if (!event.m_repeat)
		{
			EndEditing(event.m_keyCode == GLFW_KEY_ESCAPE ? EditEndReason::Cancelled : EditEndReason::Submitted);
		}

		return true;
	}

	if (event.m_keyCode == GLFW_KEY_A && (event.m_modifiers & GLFW_MOD_CONTROL) != 0
		&& (event.m_modifiers & (GLFW_MOD_ALT | GLFW_MOD_SUPER)) == 0)
	{
		SelectAll();
		return true;
	}

	if (command)
	{
		if ((event.m_modifiers & GLFW_MOD_CONTROL) != 0 && (event.m_modifiers & (GLFW_MOD_ALT | GLFW_MOD_SUPER)) == 0)
		{
			if (event.m_keyCode == GLFW_KEY_Z)
			{
				if (shift)
				{
					Redo();
				}
				else
				{
					Undo();
				}

				return true;
			}

			if (!shift)
			{
				switch (event.m_keyCode)
				{
				case GLFW_KEY_C:
				case GLFW_KEY_INSERT:
					CopyText();
					return true;
				case GLFW_KEY_X:
					CutText();
					return true;
				case GLFW_KEY_V:
					PasteText();
					return true;
				case GLFW_KEY_Y:
					Redo();
					return true;
				}
			}
		}

		return false;
	}

	if (shift && event.m_keyCode == GLFW_KEY_INSERT)
	{
		PasteText();
		return true;
	}

	if (shift && event.m_keyCode == GLFW_KEY_DELETE)
	{
		CutText();
		return true;
	}

	switch (event.m_keyCode)
	{
	case GLFW_KEY_LEFT:
		MoveCursor(HasSelection() && !shift ? GetSelectionFromColumn() : m_cursorColumn - 1, shift);
		return true;
	case GLFW_KEY_RIGHT:
		MoveCursor(HasSelection() && !shift ? GetSelectionToColumn() : m_cursorColumn + 1, shift);
		return true;
	case GLFW_KEY_HOME:
		MoveCursor(0, shift);
		return true;
	case GLFW_KEY_END:
		MoveCursor(static_cast<int>(m_text.Length()), shift);
		return true;
	case GLFW_KEY_BACKSPACE:
	case GLFW_KEY_DELETE:
		if (m_editable)
		{
			if (!HasSelection())
			{
				int const adjacent = event.m_keyCode == GLFW_KEY_BACKSPACE ? m_cursorColumn - 1 : m_cursorColumn + 1;
				m_selectionAnchor  = std::clamp(adjacent, 0, static_cast<int>(m_text.Length()));
			}

			ReplaceSelection(String());
		}

		return true;
	default:
		return false;
	}
}

void LineEdit::DrawContent()
{
	AABB2 const content = GetContentRect();
	if (content.GetWidth() <= 0.f || content.GetHeight() <= 0.f)
	{
		return;
	}

	auto drawClippedRect = [this, &content](AABB2 const& rect, Color const& color)
	{
		AABB2 const clipped(
			std::max(rect.m_mins.x, content.m_mins.x),
			std::max(rect.m_mins.y, content.m_mins.y),
			std::min(rect.m_maxs.x, content.m_maxs.x),
			std::min(rect.m_maxs.y, content.m_maxs.y));
		if (clipped.GetWidth() > 0.f && clipped.GetHeight() > 0.f)
		{
			DrawRect(clipped, color);
		}
	};

	float const height  = std::abs(m_line.m_config.m_ascent) + std::abs(m_line.m_config.m_descent);
	float const top     = content.m_mins.y + (content.GetHeight() - height) * 0.5f;
	float const originX = content.m_mins.x - m_scrollOffset;
	if (HasSelection())
	{
		drawClippedRect(
			AABB2(
				originX + GetColumnPosition(GetSelectionFromColumn()),
				top,
				originX + GetColumnPosition(GetSelectionToColumn()),
				top + height),
			Color(45, 100, 165));
	}

	bool const      placeholder = m_text.IsEmpty();
	TextLine const& line        = placeholder ? m_placeholderLine : m_line;
	Color const     textColor = placeholder || !m_editable || m_disabled ? Color(145, 155, 170) : Color(235, 240, 245);
	if (m_font.IsValid() && m_font->GetFontRID().IsValid() && g_engine != nullptr && g_engine->m_textServer != nullptr)
	{
		float const lineHeight = std::abs(line.m_config.m_ascent) + std::abs(line.m_config.m_descent);
		float const baseline   = content.m_mins.y + (content.GetHeight() - lineHeight) * 0.5f + line.m_config.m_ascent;
		for (TextLineGlyph const& glyph : line.m_glyphs)
		{
			GlyphTexture const* texture =
				g_engine->m_textServer->GetOrCreateGlyphTexture(m_font->GetFontRID(), glyph.m_glyphIndex, m_fontSize);
			if (texture == nullptr || !texture->m_texture.IsValid())
			{
				continue;
			}

			GlyphImage const& bitmap = texture->m_bitmap;
			Vector2 const     position(
				content.m_mins.x + glyph.m_posX + static_cast<float>(bitmap.m_offsetX)
					- (placeholder ? 0.f : m_scrollOffset),
				baseline + static_cast<float>(bitmap.m_offsetY));
			Vector2 const size(static_cast<float>(bitmap.m_width), static_cast<float>(bitmap.m_height));
			Vector2 const clippedMin(std::max(position.x, content.m_mins.x), std::max(position.y, content.m_mins.y));
			Vector2 const clippedMax(
				std::min(position.x + size.x, content.m_maxs.x),
				std::min(position.y + size.y, content.m_maxs.y));
			if (clippedMax.x <= clippedMin.x || clippedMax.y <= clippedMin.y)
			{
				continue;
			}

			Rect2 target;
			target.m_position = clippedMin;
			target.m_size     = clippedMax - clippedMin;
			Rect2 source;
			source.m_position = clippedMin - position;
			source.m_size     = target.m_size;
			DrawTextureRectRegion(texture->m_texture, target, source, textColor);
		}
	}

	if (m_editable && !m_disabled && HasFocus() && GetViewport()->IsWindowFocused())
	{
		float const cursorHeight = height > 0.f ? height : static_cast<float>(m_fontSize);
		float const cursorTop    = content.m_mins.y + (content.GetHeight() - cursorHeight) * 0.5f;
		float const x            = originX + GetColumnPosition(m_cursorColumn);
		drawClippedRect(AABB2(x, cursorTop, x + 1.f, cursorTop + cursorHeight), Color(245, 210, 90));
	}
}

void LineEdit::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_EnterTree:
		UpdateTextLayout();
		break;
	case Notification_Resized:
		EnsureCursorVisible();
		QueueRedraw();
		break;
	case Notification_FocusEnter:
		QueueRedraw();
		BeginEditing();
		break;
	case Notification_FocusExit:
		Deselect();
		m_selecting = false;
		EndEditing(EditEndReason::FocusLost);
		break;
	case Notification_GuiInputCancelled:
	case Notification_ExitTree:
	case Notification_PreDelete:
		m_selecting = false;
		QueueRedraw();
		EndEditing();
		break;
	case Notification_VisibilityChanged:
		QueueRedraw();
		if (!IsVisibleInHierarchy())
		{
			EndEditing();
		}

		break;
	case Notification_Draw:
	{
		DrawRect(GetLocalRect(), HasFocus() ? Color(245, 210, 90) : Color(115, 130, 150));
		AABB2 inner = GetLocalRect();
		inner.Shrink(1.f);
		if (inner.GetWidth() > 0.f && inner.GetHeight() > 0.f)
		{
			DrawRect(inner, m_disabled ? Color(38, 40, 46) : Color(28, 34, 45));
		}

		DrawContent();
		break;
	}
	}
}
