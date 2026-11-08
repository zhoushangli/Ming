#include "MingEngine/Editor/UI/EditorSpinSlider.hpp"

#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

EditorSpinSlider::EditorSpinSlider()
{
	SetReady(true);
	SetProcess(true);
	SetFocusMode(FocusMode::All);
	SetSerializable(false);
	m_edit = MemNew<LineEdit>();
	m_edit->SetMinimumCharacterWidth(1);
	m_edit->SetThemeTypeVariation("EditorPropertyEdit");
	m_edit->SetFocusMode(FocusMode::None);
	m_edit->SetMouseFilter(MouseFilter::Ignore);
	AddNode(m_edit);
	m_edit->SetEditingStartedCallback(
		[this](String const& text)
		{
			m_startText = text;
			m_edit->SelectAll();
		});
	m_edit->SetEditingEndedCallback(
		[this](String const& text, LineEdit::EditEndReason reason)
		{
			FinishTextEdit(text, reason);
		});
	m_errorLabel = MemNew<Label>();
	m_errorLabel->SetThemeTypeVariation("SpinBoxError");
	m_errorLabel->SetMouseFilter(MouseFilter::Ignore);
	m_errorLabel->SetVisible(false);
	AddNode(m_errorLabel);
	SetValueNoSignal(m_value);
}

EditorSpinSlider::~EditorSpinSlider()
{
	FinishDrag(false);
}

bool EditorSpinSlider::Configure(bool integer, std::string const& range)
{
	m_integer    = integer;
	m_min        = integer ? static_cast<double>(std::numeric_limits<int>::min())
						   : -static_cast<double>(std::numeric_limits<float>::max());
	m_max        = integer ? static_cast<double>(std::numeric_limits<int>::max())
						   : static_cast<double>(std::numeric_limits<float>::max());
	m_step       = integer ? 1.0 : 0.1;
	m_configured = true;
	if (!range.empty())
	{
		size_t const first  = range.find(',');
		size_t const second = first == std::string::npos ? first : range.find(',', first + 1);
		Variant      minimum, maximum, step;
		m_configured = first != std::string::npos && second != std::string::npos
					   && SpinBox::ParseValue(range.substr(0, first), integer, minimum)
					   && SpinBox::ParseValue(range.substr(first + 1, second - first - 1), integer, maximum)
					   && SpinBox::ParseValue(range.substr(second + 1), integer, step);
		if (m_configured)
		{
			m_min        = integer ? static_cast<double>(minimum.As<int>()) : static_cast<double>(minimum.As<float>());
			m_max        = integer ? static_cast<double>(maximum.As<int>()) : static_cast<double>(maximum.As<float>());
			m_step       = integer ? static_cast<double>(step.As<int>()) : static_cast<double>(step.As<float>());
			m_configured = m_min <= m_max && m_step > 0.0;
		}
	}

	m_edit->SetEditable(m_configured);
	SetValueNoSignal(integer ? Variant(0) : Variant(0.f));
	SetError(m_configured ? "" : "Invalid range hint");
	return m_configured;
}

void EditorSpinSlider::SetValueNoSignal(Variant const& value)
{
	if ((m_integer && !value.Is<int>()) || (!m_integer && !value.Is<float>()))
	{
		return;
	}

	m_value = value;
	if (!m_edit->IsEditing())
	{
		String const text(SpinBox::FormatValue(value));
		if (m_edit->GetText() != text)
		{
			m_edit->SetText(text);
		}
	}
}

void EditorSpinSlider::SetDragCallbacks(
	std::function<void()> started, std::function<void(Variant const&)> preview, std::function<void(bool)> ended)
{
	m_started = std::move(started);
	m_preview = std::move(preview);
	m_ended   = std::move(ended);
}

Vector2 EditorSpinSlider::GetInternalMinimumSize() const
{
	Vector2 size = m_edit->GetCombinedMinimumSize();
	if (!m_error.empty())
	{
		size.y += m_errorLabel->GetCombinedMinimumSize().y + 2.f;
	}
	return size;
}

CursorShape EditorSpinSlider::GetCursorShape(Vector2 const&) const
{
	return m_configured ? CursorShape::ResizeHorizontal : CursorShape::Arrow;
}

void EditorSpinSlider::Layout()
{
	if (m_edit == nullptr || m_errorLabel == nullptr)
	{
		return;
	}

	float const height = m_edit->GetCombinedMinimumSize().y;
	m_edit->UpdateSize(Vector2(GetSize().x, height));
	m_errorLabel->SetPosition(Vector2(0.f, height + 2.f));
	m_errorLabel->UpdateSize(Vector2(GetSize().x, m_errorLabel->GetCombinedMinimumSize().y));
}

void EditorSpinSlider::SetError(std::string const& error)
{
	m_error = error;
	m_errorLabel->SetText(error);
	m_errorLabel->SetVisible(!error.empty());
	PropagateMinimumSizeChanged();
	Layout();
}

void EditorSpinSlider::BeginTextEdit()
{
	if (m_finishing || !m_configured || m_edit->IsEditing())
	{
		return;
	}

	SetFocusMode(FocusMode::None);
	m_edit->SetMouseFilter(MouseFilter::Stop);
	m_edit->SetFocusMode(FocusMode::All);
	m_edit->GrabFocus();
	m_edit->SelectAll();
}

void EditorSpinSlider::FinishTextEdit(String const& text, LineEdit::EditEndReason reason)
{
	if (m_finishing)
	{
		return;
	}

	m_finishing = true;
	if (m_reader)
	{
		SetValueNoSignal(m_reader());
	}

	Variant    parsed;
	bool const submit = reason == LineEdit::EditEndReason::Submitted || reason == LineEdit::EditEndReason::FocusLost;
	if (submit && text != m_startText)
	{
		bool const   valid = SpinBox::ParseValue(text.ToUtf8(), m_integer, parsed);
		double const number =
			valid ? (m_integer ? static_cast<double>(parsed.As<int>()) : static_cast<double>(parsed.As<float>())) : 0.0;
		if (valid && number >= m_min && number <= m_max)
		{
			SetError("");
			bool const changed = parsed != m_value;
			SetValueNoSignal(parsed);
			if (changed && m_changed)
			{
				m_changed(parsed);
			}
		}
		else
		{
			SetError(valid ? "Value outside range" : (m_integer ? "Invalid integer" : "Invalid finite number"));
		}
	}

	SetValueNoSignal(m_value);
	m_edit->SetFocusMode(FocusMode::None);
	m_edit->SetMouseFilter(MouseFilter::Ignore);
	SetFocusMode(FocusMode::All);
	if (reason == LineEdit::EditEndReason::Submitted || reason == LineEdit::EditEndReason::Cancelled)
	{
		GrabFocus();
	}
	m_finishing = false;
}

void EditorSpinSlider::FinishDrag(bool accept)
{
	m_attempt = false;
	if (!m_dragging)
	{
		return;
	}

	m_dragging = false;
	if (g_engine != nullptr && g_engine->m_inputSystem != nullptr)
	{
		g_engine->m_inputSystem->SetCursorMode(CursorMode::POINTER);
		if (g_engine->m_windowSystem != nullptr && g_engine->m_windowSystem->GetGLFWWindow() != nullptr)
		{
			glfwSetCursorPos(g_engine->m_windowSystem->GetGLFWWindow(), m_mouseStart.x, m_mouseStart.y);
		}
	}

	if (!accept)
	{
		SetValueNoSignal(m_original);
	}
	auto ended = m_ended;
	if (ended)
	{
		ended(accept);
	}
}

void EditorSpinSlider::EndEditing(LineEdit::EditEndReason reason)
{
	FinishDrag(false);
	m_edit->EndEditing(reason);
}

bool EditorSpinSlider::GuiInput(InputEvent const& event)
{
	if (!m_configured)
	{
		return false;
	}

	switch (event.m_type)
	{
	case InputEventType::MouseButton:
		if (event.m_mouseButton == GLFW_MOUSE_BUTTON_RIGHT && event.m_pressed && m_attempt)
		{
			FinishDrag(false);
			return true;
		}
		if (event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (event.m_pressed)
			{
				m_focusPending = false;
				m_attempt      = true;
				m_distance     = 0.0;
				m_original     = m_value;
				m_mouseStart   = GetGlobalPosition() + event.m_position;
			}
			else if (m_attempt)
			{
				bool const dragged = m_dragging;
				FinishDrag(true);
				if (!dragged)
				{
					BeginTextEdit();
				}
			}
			return true;
		}
		break;
	case InputEventType::MouseMotion:
		if (m_attempt)
		{
			float const scale = std::max(1.f, GetThemeConstant("icon_size", "Button") / 20.f);
			m_distance += event.m_relative.x * ((event.m_modifiers & GLFW_MOD_SHIFT) != 0 && m_dragging ? 0.1 : 1.0);
			if (!m_dragging && std::abs(m_distance) > 4.f * scale)
			{
				m_dragging = true;
				if (g_engine != nullptr && g_engine->m_inputSystem != nullptr)
				{
					g_engine->m_inputSystem->SetCursorMode(CursorMode::FPS);
				}
				if (m_started)
				{
					m_started();
				}
			}

			if (m_dragging)
			{
				double const start =
					m_integer ? static_cast<double>(m_original.As<int>()) : static_cast<double>(m_original.As<float>());
				double value = start + m_distance * std::max(m_step, m_integer ? 1.0 : 0.1) / scale;
				value        = std::round(value / m_step) * m_step;
				if (!m_integer && (event.m_modifiers & GLFW_MOD_CONTROL) != 0)
				{
					value = std::round(value);
				}
				value              = std::clamp(value, m_min, m_max);
				Variant const next = m_integer ? Variant(static_cast<int>(value)) : Variant(static_cast<float>(value));
				SetValueNoSignal(next);
				SetError("");
				if (m_preview)
				{
					m_preview(next);
				}
			}
			return true;
		}
		break;
	case InputEventType::Key:
		if (event.m_pressed && !event.m_repeat)
		{
			if (event.m_keyCode == GLFW_KEY_ESCAPE && m_attempt)
			{
				FinishDrag(false);
				return true;
			}
			if (event.m_keyCode == GLFW_KEY_ENTER || event.m_keyCode == GLFW_KEY_KP_ENTER)
			{
				BeginTextEdit();
				return true;
			}
		}
		break;
	default:
		break;
	}
	return false;
}

void EditorSpinSlider::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Resized:
	case Notification_Ready:
	case Notification_ThemeChanged:
		Layout();
		break;
	case Notification_FocusEnter:
		m_focusPending = !m_finishing;
		break;
	case Notification_Process:
		if (m_focusPending)
		{
			m_focusPending = false;
			if (HasFocus() && !m_attempt)
			{
				BeginTextEdit();
			}
		}
		break;
	case Notification_FocusExit:
	case Notification_GuiInputCancelled:
	case Notification_ExitTree:
	case Notification_PreDelete:
		FinishDrag(false);
		break;
	case Notification_VisibilityChanged:
		if (!IsVisibleInHierarchy())
		{
			EndEditing();
		}
		break;
	}
}
