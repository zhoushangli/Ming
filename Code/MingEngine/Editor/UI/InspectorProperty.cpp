#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"

#include "MingEngine/Editor/UI/InspectorPropertyBool.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyColor.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyEuler.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyFloat.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyInt.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyResource.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyString.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyVec3.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyVec2.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyEnum.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"

#include <cctype>
#include <cmath>

InspectorProperty::InspectorProperty(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
	: m_info(std::move(info)), m_labelId(std::move(labelId)), m_onValueChanged(std::move(changed))
{
	m_root   = MemNew<VBoxContainer>();
	m_rootID = m_root->GetObjectID();
	m_root->SetSerializable(false);
	m_root->SetSeparation(3.f);
	auto* title = MemNew<Label>();
	title->SetText(GetDisplayName());
	title->SetFontSize(14);
	m_root->AddNode(title);
	m_error = MemNew<Label>();
	m_error->SetFontSize(11);
	m_error->SetVisible(false);
	m_root->AddNode(m_error);
}

InspectorProperty::~InspectorProperty()
{
	m_onValueChanged = {};
	m_reader         = {};
	if (GetControl() == nullptr)
	{
		return;
	}

	EndEditing();
	if (m_text != nullptr)
	{
		m_text->SetEditingEndedCallback({});
		m_text->SetEditingStartedCallback({});
	}

	for (SpinBox* spin : m_spins)
	{
		spin->SetValueChangedCallback({});
		spin->SetValueReader({});
	}

	if (m_button != nullptr)
	{
		m_button->SetToggledCallback({});
		m_button->SetPressedCallback({});
	}

	if (m_options != nullptr)
	{
		m_options->SetValueChangedCallback({});
	}

	m_root->SetVisible(false);
	m_root->QueueFree();
}

Control* InspectorProperty::GetControl() const
{
	return ObjectDatabase::GetInstance<Control>(m_rootID);
}

// Preserve immediate-mode import option editing until the Import panel is migrated.
// e.g. ImportPanel still calls Render(context, optionValue) for its existing fields.
void InspectorProperty::Render(EditorUIContext&, Variant const& value)
{
	Variant           next   = value;
	bool              edited = false;
	std::string const label  = GetDisplayName();
	char const*       id     = m_labelId.c_str();
	switch (value.GetType())
	{
	case Variant::Type::Bool:
		edited = EditorUIWidgets::PropertyBool(label, id, next.As<bool>());
		break;
	case Variant::Type::Int:
		edited = EditorUIWidgets::PropertyInt(label, id, next.As<int>());
		break;
	case Variant::Type::Float:
		edited = EditorUIWidgets::PropertyFloat(label, id, next.As<float>());
		break;
	case Variant::Type::String:
	{
		std::string text = value.As<String>().ToUtf8();
		edited           = EditorUIWidgets::PropertyString(label, id, text);
		next             = Variant(text);
		break;
	}
	case Variant::Type::Vector3:
		edited = EditorUIWidgets::PropertyVec3(label, id, next.As<Vector3>());
		break;
	case Variant::Type::EulerAngles:
		edited = EditorUIWidgets::PropertyEuler(label, id, next.As<EulerAngles>());
		break;
	case Variant::Type::Color:
		edited = EditorUIWidgets::PropertyColor(label, id, next.As<Color>());
		break;
	default:
		break;
	}

	if (edited)
	{
		EmitValueChanged(next);
	}
}

std::string InspectorProperty::GetDisplayName() const
{
	std::string result     = m_info.m_name;
	bool        capitalize = true;
	for (char& ch : result)
	{
		if (ch == '_')
		{
			ch         = ' ';
			capitalize = true;
		}
		else if (capitalize)
		{
			ch         = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
			capitalize = false;
		}
	}

	return result;
}

void InspectorProperty::SetError(std::string const& message)
{
	m_error->SetText(message);
	m_error->SetVisible(!message.empty());
}

Variant InspectorProperty::ReadValue() const
{
	return m_reader ? m_reader() : m_value;
}

void InspectorProperty::EmitValueChanged(Variant const& value)
{
	auto callback = m_onValueChanged;
	if (value.GetType() == m_info.m_type && callback)
	{
		callback(value);
	}
}

void InspectorProperty::BuildNumeric()
{
	auto* spin = MemNew<SpinBox>();
	spin->Configure(
		m_info.m_type == Variant::Type::Int,
		m_info.m_hint == PropertyInfo::Hint::Range ? (m_info.m_hintData.empty() ? "," : m_info.m_hintData) : "");
	spin->SetValueReader(
		[this]()
		{
			return ReadValue();
		});
	spin->SetValueChangedCallback(
		[this](Variant const& value)
		{
			EmitValueChanged(value);
		});
	m_root->AddNode(spin);
	m_spins.push_back(spin);
}

void InspectorProperty::BuildComponents()
{
	int const count = m_info.m_type == Variant::Type::Vector2 ? 2 : (m_info.m_type == Variant::Type::Color ? 4 : 3);
	if (m_info.m_type == Variant::Type::Color)
	{
		m_color = MemNew<ColorRect>();
		m_color->SetCustomMinimumSize(Vector2(0.f, 22.f));
		m_root->AddNode(m_color);
	}

	char const* names[]  = { "X", "Y", "Z", "W" };
	char const* angles[] = { "Roll (deg)", "Pitch (deg)", "Yaw (deg)" };
	char const* colors[] = { "R", "G", "B", "A" };
	for (int i = 0; i < count; ++i)
	{
		auto* row   = MemNew<HBoxContainer>();
		auto* label = MemNew<Label>();
		label->SetText(
			m_info.m_type == Variant::Type::EulerAngles
				? angles[i]
				: (m_info.m_type == Variant::Type::Color ? colors[i] : names[i]));
		label->SetCustomMinimumSize(Vector2(m_info.m_type == Variant::Type::EulerAngles ? 98.f : 20.f, 0.f));
		row->AddNode(label);
		auto* spin = MemNew<SpinBox>();
		spin->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		spin->Configure(
			m_info.m_type == Variant::Type::Color,
			m_info.m_type == Variant::Type::Color
				? "0,255,1"
				: (m_info.m_hint == PropertyInfo::Hint::Range ? (m_info.m_hintData.empty() ? "," : m_info.m_hintData)
															  : ""));
		spin->SetValueReader(
			[this, i]()
			{
				return ReadComponent(i);
			});
		spin->SetValueChangedCallback(
			[this, i](Variant const& value)
			{
				SetComponent(i, value);
			});
		row->AddNode(spin);
		m_root->AddNode(row);
		m_spins.push_back(spin);
	}
}

Variant InspectorProperty::ReadComponent(int index) const
{
	return GetComponentValue(ReadValue(), index);
}

Variant InspectorProperty::GetComponentValue(Variant const& value, int index)
{
	if (value.Is<Vector2>())
	{
		Vector2 const& v = value.As<Vector2>();
		return Variant(index == 0 ? v.x : v.y);
	}

	if (value.Is<Vector3>())
	{
		Vector3 const& v = value.As<Vector3>();
		return Variant(index == 0 ? v.x : (index == 1 ? v.y : v.z));
	}

	if (value.Is<EulerAngles>())
	{
		EulerAngles const& v = value.As<EulerAngles>();
		return Variant(index == 0 ? v.m_rollDegrees : (index == 1 ? v.m_pitchDegrees : v.m_yawDegrees));
	}

	if (value.Is<Color>())
	{
		Color const& v = value.As<Color>();
		return Variant(static_cast<int>(index == 0 ? v.r : (index == 1 ? v.g : (index == 2 ? v.b : v.a))));
	}

	return {};
}

void InspectorProperty::SetComponent(int index, Variant const& component)
{
	Variant value = ReadValue();
	if (value.Is<Vector2>())
	{
		Vector2& v               = value.As<Vector2>();
		(index == 0 ? v.x : v.y) = component.As<float>();
	}
	else if (value.Is<Vector3>())
	{
		Vector3& v      = value.As<Vector3>();
		float    number = component.As<float>();
		if (m_info.m_name == "scale" && std::abs(number) < 1e-5f)
		{
			number = 1e-5f;
		}

		(index == 0 ? v.x : (index == 1 ? v.y : v.z)) = number;
	}
	else if (value.Is<EulerAngles>())
	{
		EulerAngles& v                                                                    = value.As<EulerAngles>();
		(index == 0 ? v.m_rollDegrees : (index == 1 ? v.m_pitchDegrees : v.m_yawDegrees)) = component.As<float>();
	}
	else if (value.Is<Color>())
	{
		Color& v = value.As<Color>();
		(index == 0 ? v.r : (index == 1 ? v.g : (index == 2 ? v.b : v.a))) =
			static_cast<unsigned char>(component.As<int>());
	}
	else
	{
		return;
	}

	EmitValueChanged(value);
}

void InspectorProperty::BuildBool()
{
	m_button = MemNew<Button>();
	m_button->SetToggleMode(true);
	m_button->SetToggledCallback(
		[this](bool pressed)
		{
			EmitValueChanged(Variant(pressed));
		});
	m_root->AddNode(m_button);
}

void InspectorProperty::BuildEnum()
{
	m_options = MemNew<OptionButton>();
	if (!m_options->SetOptions(m_info.m_hintData))
	{
		SetError("Invalid enum hint");
	}

	m_options->SetValueChangedCallback(
		[this](int value)
		{
			EmitValueChanged(Variant(value));
		});
	m_root->AddNode(m_options);
}

void InspectorProperty::BuildText(bool resource)
{
	m_text = MemNew<LineEdit>();
	m_text->SetMinimumCharacterWidth(1);
	m_text->SetEditable(!resource);
	m_root->AddNode(m_text);
	if (resource)
	{
		m_button = MemNew<Button>();
		m_button->SetText("Clear");
		m_button->SetPressedCallback(
			[this]()
			{
				EmitValueChanged(Variant(static_cast<Object*>(nullptr)));
			});
		m_root->AddNode(m_button);
		return;
	}

	m_text->SetEditingEndedCallback(
		[this](String const& text, LineEdit::EditEndReason reason)
		{
			if ((reason == LineEdit::EditEndReason::Submitted || reason == LineEdit::EditEndReason::FocusLost)
				&& text != m_editStartText)
			{
				VirtualPath path;
				if (m_info.m_hint == PropertyInfo::Hint::Path && !text.IsEmpty()
					&& !VirtualPath::TryParse(text.ToUtf8(), path))
				{
					SetError("Invalid res:// path");
				}
				else
				{
					SetError("");
					EmitValueChanged(Variant(text));
				}
			}

			UpdateValue(ReadValue());
		});
	m_text->SetEditingStartedCallback(
		[this](String const& text)
		{
			m_editStartText = text;
		});
}

void InspectorProperty::UpdateValue(Variant const& value)
{
	if (value.GetType() != m_info.m_type || GetControl() == nullptr)
	{
		return;
	}

	m_value = value;
	for (int i = 0; i < static_cast<int>(m_spins.size()); ++i)
	{
		m_spins[i]->SetValueNoSignal(
			m_info.m_type == Variant::Type::Int || m_info.m_type == Variant::Type::Float ? value
																						 : GetComponentValue(value, i));
	}

	if (m_color != nullptr)
	{
		m_color->SetColor(value.As<Color>());
	}

	if (m_options != nullptr)
	{
		m_options->SetValueNoSignal(value.As<int>());
	}

	if (m_info.m_type == Variant::Type::Bool && m_button != nullptr)
	{
		m_button->SetPressedNoSignal(value.As<bool>());
		m_button->SetText(value.As<bool>() ? "On" : "Off");
	}

	if (m_text != nullptr && !m_text->IsEditing())
	{
		String text;
		if (value.Is<String>())
		{
			text = value.As<String>();
		}
		else if (value.Is<Object*>())
		{
			auto* resource = dynamic_cast<Resource*>(value.As<Object*>());
			text           = resource != nullptr ? String(resource->GetVirtualPath().GetString()) : String("<empty>");
			m_button->SetDisabled(resource == nullptr);
		}

		if (text != m_text->GetText())
		{
			m_text->SetText(text);
		}
	}
}

void InspectorProperty::EndEditing(LineEdit::EditEndReason reason)
{
	if (GetControl() == nullptr)
	{
		return;
	}

	for (SpinBox* spin : m_spins)
	{
		spin->EndEditing(reason);
	}

	if (m_text != nullptr)
	{
		m_text->EndEditing(reason);
	}

	if (m_options != nullptr)
	{
		m_options->CloseOptions(false);
	}
}

bool InspectorProperty::Contains(Vector2 position) const
{
	Control* control = GetControl();
	return control != nullptr && control->IsPointInside(position - control->GetGlobalPosition());
}

bool InspectorProperty::CanAssignResource(VirtualPath const& path) const
{
	if (m_info.m_type != Variant::Type::ObjectPtr || m_info.m_hint != PropertyInfo::Hint::ResourceType)
	{
		return false;
	}

	Ref<Resource> resource = ResourceLoader::Load(path);
	return resource.IsValid() && ClassDatabase::IsSubclassOf(resource->GetClassName(), m_info.m_hintData);
}

bool InspectorProperty::AssignResource(VirtualPath const& path)
{
	Ref<Resource> resource = ResourceLoader::Load(path);
	if (!resource.IsValid() || !CanAssignResource(path))
	{
		SetError("Resource type mismatch");
		return false;
	}

	SetError("");
	EmitValueChanged(Variant(resource));
	return true;
}

InspectorProperty* InspectorProperty::Create(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
{
	if (info.m_type == Variant::Type::ObjectPtr && info.m_hint == PropertyInfo::Hint::ResourceType)
	{
		return new InspectorPropertyResource(std::move(info), std::move(labelId), std::move(changed));
	}

	if (info.m_type == Variant::Type::Int && info.m_hint == PropertyInfo::Hint::Enum)
	{
		return new InspectorPropertyEnum(std::move(info), std::move(labelId), std::move(changed));
	}

	switch (info.m_type)
	{
	case Variant::Type::Bool:
		return new InspectorPropertyBool(std::move(info), std::move(labelId), std::move(changed));
	case Variant::Type::Int:
		return new InspectorPropertyInt(std::move(info), std::move(labelId), std::move(changed));
	case Variant::Type::Float:
		return new InspectorPropertyFloat(std::move(info), std::move(labelId), std::move(changed));
	case Variant::Type::String:
		return new InspectorPropertyString(std::move(info), std::move(labelId), std::move(changed));
	case Variant::Type::Vector2:
		return new InspectorPropertyVec2(std::move(info), std::move(labelId), std::move(changed));
	case Variant::Type::Vector3:
		return new InspectorPropertyVec3(std::move(info), std::move(labelId), std::move(changed));
	case Variant::Type::EulerAngles:
		return new InspectorPropertyEuler(std::move(info), std::move(labelId), std::move(changed));
	case Variant::Type::Color:
		return new InspectorPropertyColor(std::move(info), std::move(labelId), std::move(changed));
	default:
		return nullptr;
	}
}
