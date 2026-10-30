#pragma once

#include "MingEngine/Engine/Render/TextServer.hpp"
#include "MingEngine/Scene/GUI/Control.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"

#include <functional>

class LineEdit : public Control
{
	MCLASS(LineEdit, Control);

public:
	enum class EditEndReason
	{
		Submitted,
		Cancelled,
		FocusLost,
		Interrupted
	};

public:
	LineEdit();
	~LineEdit() override;

	String const& GetText() const { return m_text; }
	String const& GetPlaceholder() const { return m_placeholder; }

	Ref<FontResource> GetFont() const { return m_font; }
	int               GetFontSize() const { return m_fontSize; }
	Vector2           GetPadding() const { return m_padding; }
	Vector2           GetInternalMinimumSize() const override;

	bool IsEditable() const { return m_editable; }
	bool IsDisabled() const override { return m_disabled; }
	bool IsEditing() const { return m_editing; }

	int GetCursorColumn() const { return m_cursorColumn; }
	// Return the nonnegative pixel distance subtracted from text positions when drawing.
	// e.g. A text position of 200 with an offset of 150 appears 50 pixels into the content area.
	float GetScrollOffset() const { return m_scrollOffset; }

	bool   HasSelection() const { return m_selectionAnchor != m_cursorColumn; }
	int    GetSelectionFromColumn() const;
	int    GetSelectionToColumn() const;
	String GetSelectedText() const;

	bool   HasUndo() const;
	bool   HasRedo() const;

	// Set filtered single-line content, reset editing state, and leave the change callback silent.
	// e.g. SetText(U"Name\n") stores U"Name" with the cursor at column zero.
	void SetText(String const& text);
	void SetPlaceholder(String const& text);

	void SetFont(Ref<FontResource> const& font);
	void SetFontSize(int fontSize);
	void SetPadding(Vector2 const& padding);
	void SetMinimumCharacterWidth(int width);

	void SetEditable(bool editable);
	void SetDisabled(bool disabled);

	void SetCursorColumn(int column);

	void SetTextChangedCallback(std::function<void(String const&)> callback)
	{
		m_textChangedCallback = std::move(callback);
	}
	void SetEditingStartedCallback(std::function<void(String const&)> callback)
	{
		m_editingStartedCallback = std::move(callback);
	}
	void SetEditingEndedCallback(std::function<void(String const&, EditEndReason)> callback)
	{
		m_editingEndedCallback = std::move(callback);
	}

	// End the active edit before notifying its owner, without changing text or focus.
	// e.g. EndEditing() interrupts an edit before rebinding this field to another object.
	void EndEditing(EditEndReason reason = EditEndReason::Interrupted);

	// Keep from as the selection anchor and move the cursor to to, clamping both to the text.
	// e.g. Select(4, 1) selects columns [1, 4) with the cursor at column one.
	void Select(int from, int to);
	void SelectAll();
	void Deselect();

	void CopyText();
	void CutText();
	void PasteText();

	void Undo();
	void Redo();

	bool GuiInput(InputEvent const& event) override;

protected:
	static void BindMethods();

	void OnNotification(int notification);

private:
	int   GetColumnAtPosition(float localX) const;
	float GetColumnPosition(int column) const;
	AABB2 GetContentRect() const;

	static bool   IsTextCodepoint(char32_t codepoint);
	static String FilterText(String const& text);

	bool BeginEditing();
	void RestoreEditState(String const& text, int cursorColumn, float scrollOffset);
	void EmitTextChanged();

	void MoveCursor(int column, bool select);
	void ReplaceSelection(String const& text);

	void UpdateTextLayout();
	void EnsureCursorVisible();
	void DrawContent();

private:
	String                                            m_text;
	String                                            m_placeholder;
	Ref<FontResource>                                 m_font;
	TextLine                                          m_line;
	TextLine                                          m_placeholderLine;
	Vector2                                           m_padding         = Vector2(8.f, 6.f);
	int                                               m_fontSize        = 16;
	int                                               m_minimumCharacterWidth = 8;
	int                                               m_cursorColumn    = 0;
	int                                               m_selectionAnchor = 0;
	float                                             m_scrollOffset    = 0.f;
	bool                                              m_editable        = true;
	bool                                              m_disabled        = false;
	bool                                              m_selecting       = false;
	bool                                              m_editing         = false;
	uint64_t                                          m_editVersion     = 0;
	std::function<void(String const&)>                m_textChangedCallback;
	std::function<void(String const&)>                m_editingStartedCallback;
	std::function<void(String const&, EditEndReason)> m_editingEndedCallback;
};
