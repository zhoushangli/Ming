#include "MingEngine/Scene/GUI/SpinBox.hpp"

#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

bool SpinBox::ParseValue(std::string const& source, bool integer, Variant& value)
{
	size_t const first = source.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
	{
		return false;
	}

	std::string text = source.substr(first, source.find_last_not_of(" \t\r\n") - first + 1);
	if (text.front() == '+')
	{
		text.erase(0, 1);
		if (text.empty() || text.front() == '+' || text.front() == '-')
		{
			return false;
		}
	}

	if (integer)
	{
		int        result = 0;
		auto const parsed = std::from_chars(text.data(), text.data() + text.size(), result);
		if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size())
		{
			return false;
		}

		value = Variant(result);
		return true;
	}

	// 1) Check decimal syntax before accepting the library's floating point parser.
	size_t index  = text.front() == '-' ? 1 : 0;
	bool   digits = false;
	while (index < text.size() && text[index] >= '0' && text[index] <= '9')
	{
		digits = true;
		++index;
	}

	if (index < text.size() && text[index] == '.')
	{
		++index;
		while (index < text.size() && text[index] >= '0' && text[index] <= '9')
		{
			digits = true;
			++index;
		}
	}

	if (!digits)
	{
		return false;
	}

	if (index < text.size() && (text[index] == 'e' || text[index] == 'E'))
	{
		++index;
		if (index < text.size() && (text[index] == '+' || text[index] == '-'))
		{
			++index;
		}

		size_t const exponentStart = index;
		while (index < text.size() && text[index] >= '0' && text[index] <= '9')
		{
			++index;
		}

		if (index == exponentStart)
		{
			return false;
		}
	}

	if (index != text.size())
	{
		return false;
	}

	float      result = 0.f;
	auto const parsed = std::from_chars(text.data(), text.data() + text.size(), result, std::chars_format::general);
	if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size() || !std::isfinite(result))
	{
		return false;
	}

	value = Variant(result);
	return true;
}

std::string SpinBox::FormatValue(Variant const& value)
{
	if (value.Is<int>())
	{
		return std::to_string(value.As<int>());
	}

	std::ostringstream stream;
	stream.imbue(std::locale::classic());
	stream << std::setprecision(std::numeric_limits<float>::max_digits10) << value.As<float>();
	return stream.str();
}

SpinBox::SpinBox()
{
	SetSeparation(2.f);
	auto* row = MemNew<HBoxContainer>();
	row->SetSeparation(2.f);
	AddNode(row);
	m_edit = MemNew<LineEdit>();
	m_edit->SetMinimumCharacterWidth(1);
	m_edit->SetHorizontalSizeFlags(Size_ExpandFill);
	row->AddNode(m_edit);

	for (int direction : { -1, 1 })
	{
		auto* button = MemNew<Button>();
		button->SetText(direction < 0 ? "-" : "+");
		button->SetFontSize(12);
		button->SetFocusMode(FocusMode::None);
		button->SetPressedCallback(
			[this, direction]()
			{
				Step(direction);
			});
		m_stepButtons[direction < 0 ? 0 : 1] = button;
		row->AddNode(button);
	}

	m_errorLabel = MemNew<Label>();
	m_errorLabel->SetFontSize(11);
	m_errorLabel->SetVisible(false);
	AddNode(m_errorLabel);
	m_edit->SetEditingEndedCallback(
		[this](String const& text, LineEdit::EditEndReason reason)
		{
			Finish(text, reason);
		});
	m_edit->SetEditingStartedCallback(
		[this](String const& text)
		{
			m_editStartText = text;
		});
	SetValueNoSignal(m_value);
}

bool SpinBox::Configure(bool integer, std::string const& range)
{
	m_integer    = integer;
	m_min        = integer ? static_cast<double>(std::numeric_limits<int>::min()) : -std::numeric_limits<float>::max();
	m_max        = integer ? static_cast<double>(std::numeric_limits<int>::max()) : std::numeric_limits<float>::max();
	m_step       = integer ? 1.0 : 0.1;
	m_configured = true;
	if (!range.empty())
	{
		size_t const comma  = range.find(',');
		size_t const second = comma == std::string::npos ? comma : range.find(',', comma + 1);
		Variant      minimum;
		Variant      maximum;
		Variant      step;
		m_configured = comma != std::string::npos && second != std::string::npos
					   && ParseValue(range.substr(0, comma), integer, minimum)
					   && ParseValue(range.substr(comma + 1, second - comma - 1), integer, maximum)
					   && ParseValue(range.substr(second + 1), integer, step);
		if (m_configured)
		{
			m_min        = integer ? static_cast<double>(minimum.As<int>()) : static_cast<double>(minimum.As<float>());
			m_max        = integer ? static_cast<double>(maximum.As<int>()) : static_cast<double>(maximum.As<float>());
			m_step       = integer ? static_cast<double>(step.As<int>()) : static_cast<double>(step.As<float>());
			m_configured = m_min <= m_max && m_step > 0.0;
		}
	}

	m_edit->SetEditable(m_configured);
	for (Button* button : m_stepButtons)
	{
		button->SetDisabled(!m_configured);
	}
	if ((integer && !m_value.Is<int>()) || (!integer && !m_value.Is<float>()))
	{
		SetValueNoSignal(integer ? Variant(0) : Variant(0.f));
	}

	SetError(m_configured ? "" : "Invalid range hint");
	return m_configured;
}

void SpinBox::SetValueNoSignal(Variant const& value)
{
	if ((!m_integer && !value.Is<float>()) || (m_integer && !value.Is<int>()))
	{
		return;
	}

	m_value = value;
	if (!m_edit->IsEditing())
	{
		String const text(FormatValue(value));
		if (m_edit->GetText() != text)
		{
			m_edit->SetText(text);
		}
	}
}

void SpinBox::SetError(std::string const& error)
{
	m_error = error;
	m_errorLabel->SetText(error);
	m_errorLabel->SetVisible(!error.empty());
}

bool SpinBox::Validate(String const& text, Variant& value)
{
	if (!m_configured)
	{
		return false;
	}

	if (!ParseValue(text.ToUtf8(), m_integer, value))
	{
		SetError(m_integer ? "Invalid integer" : "Invalid finite number");
		return false;
	}

	double const number = m_integer ? static_cast<double>(value.As<int>()) : static_cast<double>(value.As<float>());
	if (number < m_min || number > m_max)
	{
		SetError("Value outside range");
		return false;
	}

	SetError("");
	return true;
}

void SpinBox::Publish(Variant const& value)
{
	bool const changed = value != m_value;
	m_value            = value;
	SetValueNoSignal(value);
	auto callback = m_changed;
	if (changed && callback)
	{
		callback(value);
	}
}

void SpinBox::Finish(String const& text, LineEdit::EditEndReason reason)
{
	if (m_reader)
	{
		Variant const current = m_reader();
		if (current.GetType() == m_value.GetType())
		{
			m_value = current;
		}
	}

	if (reason == LineEdit::EditEndReason::Cancelled || reason == LineEdit::EditEndReason::Interrupted
		|| text == m_editStartText)
	{
		SetValueNoSignal(m_value);
		return;
	}

	Variant value;
	if (Validate(text, value))
	{
		Publish(value);
	}
	else
	{
		SetValueNoSignal(m_value);
	}
}

void SpinBox::EndEditing(LineEdit::EditEndReason reason)
{
	m_edit->EndEditing(reason);
}

void SpinBox::Step(int direction)
{
	if (!m_configured)
	{
		return;
	}

	Variant    value;
	bool const valid = Validate(m_edit->GetText(), value);
	m_edit->EndEditing(LineEdit::EditEndReason::Interrupted);
	if (!valid)
	{
		SetValueNoSignal(m_value);
		return;
	}

	double const current = m_integer ? static_cast<double>(value.As<int>()) : static_cast<double>(value.As<float>());
	double const next    = std::clamp(current + direction * m_step, m_min, m_max);
	Publish(m_integer ? Variant(static_cast<int>(next)) : Variant(static_cast<float>(next)));
}
