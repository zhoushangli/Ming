#pragma once

#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

class Label;

class EditorSpinSlider : public Control
{
	MCLASS(EditorSpinSlider, Control);

public:
	EditorSpinSlider();
	~EditorSpinSlider() override;

	Variant            GetValue() const { return m_value; }
	LineEdit*          GetLineEdit() const { return m_edit; }
	std::string const& GetError() const { return m_error; }
	bool               IsDragging() const { return m_dragging; }
	Vector2            GetInternalMinimumSize() const override;
	CursorShape        GetCursorShape(Vector2 const& position) const override;

	bool Configure(bool integer, std::string const& range = "");
	void SetValueNoSignal(Variant const& value);
	void SetValueReader(std::function<Variant()> reader) { m_reader = std::move(reader); }
	void SetValueChangedCallback(std::function<void(Variant const&)> callback) { m_changed = std::move(callback); }
	void SetDragCallbacks(
		std::function<void()> started, std::function<void(Variant const&)> preview, std::function<void(bool)> ended);
	void EndEditing(LineEdit::EditEndReason reason = LineEdit::EditEndReason::Interrupted);
	bool GuiInput(InputEvent const& event) override;

protected:
	static void BindMethods() {}
	void        OnNotification(int notification);

private:
	void BeginTextEdit();
	void FinishTextEdit(String const& text, LineEdit::EditEndReason reason);
	void FinishDrag(bool accept);
	void Layout();
	void SetError(std::string const& error);

private:
	LineEdit*                           m_edit       = nullptr;
	Label*                              m_errorLabel = nullptr;
	Variant                             m_value      = Variant(0.f);
	Variant                             m_original;
	String                              m_startText;
	std::string                         m_error;
	bool                                m_integer      = false;
	bool                                m_configured   = true;
	bool                                m_attempt      = false;
	bool                                m_dragging     = false;
	bool                                m_finishing    = false;
	bool                                m_focusPending = false;
	double                              m_min          = -3.4028234663852886e38;
	double                              m_max          = 3.4028234663852886e38;
	double                              m_step         = 0.1;
	double                              m_distance     = 0.0;
	Vector2                             m_mouseStart;
	std::function<Variant()>            m_reader;
	std::function<void(Variant const&)> m_changed;
	std::function<void()>               m_started;
	std::function<void(Variant const&)> m_preview;
	std::function<void(bool)>           m_ended;
};
