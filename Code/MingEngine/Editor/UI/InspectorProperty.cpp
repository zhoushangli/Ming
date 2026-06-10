#include "MingEngine/Editor/UI/InspectorProperty.hpp"

#include "MingEngine/Editor/UI/InspectorPropertyBool.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyEuler.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyFloat.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyInt.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyString.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyVec3.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

// Forward declarations for subclasses — included in factory below
class InspectorPropertyBool;
class InspectorPropertyInt;
class InspectorPropertyFloat;
class InspectorPropertyString;
class InspectorPropertyVec3;
class InspectorPropertyEuler;

// ——— Helper ———

static std::string SnakeToTitle(std::string const& snake)
{
	std::string result;
	result.reserve(snake.size());
	bool capitalize = true;
	for (char ch : snake)
	{
		if (ch == '_')
		{
			result += ' ';
			capitalize = true;
		}
		else if (capitalize)
		{
			result += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
			capitalize = false;
		}
		else
		{
			result += ch;
		}
	}
	return result;
}

// ——— Base class ———

InspectorProperty::InspectorProperty(PropertyInfo info, Variant value, std::string labelId)
	: m_info(std::move(info)), m_value(std::move(value)), m_labelId(std::move(labelId))
{
}

bool InspectorProperty::WasEdited() const { return m_edited; }

void InspectorProperty::Apply(Node* node)
{
	if (!m_edited || node == nullptr)
		return;

	MethodBind const* setter = m_info.GetSetter();
	if (setter != nullptr)
	{
		setter->Invoke(*node, { m_value });
	}

	// Refresh from getter
	MethodBind const* getter = m_info.GetGetter();
	if (getter != nullptr)
	{
		m_value  = getter->Invoke(*node, {});
		m_edited = false;
	}
}

char const* InspectorProperty::GetLabelId() const { return m_labelId.c_str(); }

Variant::Type InspectorProperty::GetType() const { return m_info.m_type; }

std::string InspectorProperty::GetDisplayName() const { return SnakeToTitle(m_info.m_name); }

// ——— Factory ———

InspectorProperty* InspectorProperty::Create(PropertyInfo info, Variant value, std::string labelId)
{
	switch (info.m_type)
	{
	case Variant::Type::Bool:
		return new InspectorPropertyBool(std::move(info), std::move(value), std::move(labelId));
	case Variant::Type::Int:
		return new InspectorPropertyInt(std::move(info), std::move(value), std::move(labelId));
	case Variant::Type::Float:
		return new InspectorPropertyFloat(std::move(info), std::move(value), std::move(labelId));
	case Variant::Type::String:
		return new InspectorPropertyString(std::move(info), std::move(value), std::move(labelId));
	case Variant::Type::Vec3:
		return new InspectorPropertyVec3(std::move(info), std::move(value), std::move(labelId));
	case Variant::Type::EulerAngles:
		return new InspectorPropertyEuler(std::move(info), std::move(value), std::move(labelId));
	default:
		return nullptr;
	}
}
