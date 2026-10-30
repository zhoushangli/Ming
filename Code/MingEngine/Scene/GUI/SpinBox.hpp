#pragma once

#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Core/Object/Variant.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"

class Label;
class Button;

class SpinBox : public VBoxContainer
{
	MCLASS(SpinBox, VBoxContainer);

public:
	SpinBox();

	Variant            GetValue() const { return m_value; }
	LineEdit*          GetLineEdit() const { return m_edit; }
	std::string const& GetError() const { return m_error; }

	void SetValueNoSignal(Variant const& value);

	void SetValueChangedCallback(std::function<void(Variant const&)> callback) { m_changed = std::move(callback); }
	void SetValueReader(std::function<Variant()> reader) { m_reader = std::move(reader); }

	bool Configure(bool integer, std::string const& range = "");

	void EndEditing(LineEdit::EditEndReason reason = LineEdit::EditEndReason::Interrupted);
	void Step(int direction);

	static bool        ParseValue(std::string const& text, bool integer, Variant& value);
	static std::string FormatValue(Variant const& value);

protected:
	static void BindMethods() {}

private:
	void Finish(String const& text, LineEdit::EditEndReason reason);
	void Publish(Variant const& value);

	bool Validate(String const& text, Variant& value);
	void SetError(std::string const& error);

private:
	LineEdit*                           m_edit           = nullptr;
	Label*                              m_errorLabel     = nullptr;
	Button*                             m_stepButtons[2] = {};
	Variant                             m_value          = Variant(0.f);
	bool                                m_integer        = false;
	bool                                m_configured     = true;
	double                              m_min            = -3.4028234663852886e38;
	double                              m_max            = 3.4028234663852886e38;
	double                              m_step           = 0.1;
	std::string                         m_error;
	std::function<void(Variant const&)> m_changed;
	std::function<Variant()>            m_reader;
	String                              m_editStartText;
};
