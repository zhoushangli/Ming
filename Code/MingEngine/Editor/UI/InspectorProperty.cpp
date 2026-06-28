#include "MingEngine/Editor/UI/InspectorProperty.hpp"

#include "MingEngine/Editor/UI/InspectorPropertyBool.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyEuler.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyFloat.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyInt.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyResource.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyString.hpp"
#include "MingEngine/Editor/UI/InspectorPropertyVec3.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

#include <cctype>
#include <utility>

// Forward declarations for subclasses — included in factory below
class InspectorPropertyBool;
class InspectorPropertyInt;
class InspectorPropertyFloat;
class InspectorPropertyResource;
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

InspectorProperty::InspectorProperty(
	PropertyInfo info, Node* node, std::string labelId, ValueChangedCallback onValueChanged)
	: m_info(std::move(info)), m_node(node), m_labelId(std::move(labelId)), m_onValueChanged(std::move(onValueChanged))
{
}

void InspectorProperty::Render() { RenderValue(GetCurrentValue()); }

Variant InspectorProperty::GetCurrentValue() const
{
	MethodBind const* getter = m_info.GetGetter();
	if (m_node != nullptr && getter != nullptr)
	{
		return getter->Invoke(m_node, {});
	}

	return Variant();
}

void InspectorProperty::EmitValueChanged(Variant const& value) const
{
	if (m_onValueChanged)
	{
		m_onValueChanged(value);
	}
}

char const* InspectorProperty::GetLabelId() const { return m_labelId.c_str(); }

Variant::Type InspectorProperty::GetType() const { return m_info.m_type; }

std::string InspectorProperty::GetDisplayName() const { return SnakeToTitle(m_info.m_name); }

// ——— Factory ———

InspectorProperty*
InspectorProperty::Create(PropertyInfo info, Node* node, std::string labelId, ValueChangedCallback onValueChanged)
{
	if (info.m_type == Variant::Type::ObjectPtr && info.m_hint == PropertyInfo::Hint::ResourceType)
	{
		return new InspectorPropertyResource(std::move(info), node, std::move(labelId), std::move(onValueChanged));
	}

	switch (info.m_type)
	{
	case Variant::Type::Bool:
		return new InspectorPropertyBool(std::move(info), node, std::move(labelId), std::move(onValueChanged));
	case Variant::Type::Int:
		return new InspectorPropertyInt(std::move(info), node, std::move(labelId), std::move(onValueChanged));
	case Variant::Type::Float:
		return new InspectorPropertyFloat(std::move(info), node, std::move(labelId), std::move(onValueChanged));
	case Variant::Type::String:
		return new InspectorPropertyString(std::move(info), node, std::move(labelId), std::move(onValueChanged));
	case Variant::Type::Vec3:
		return new InspectorPropertyVec3(std::move(info), node, std::move(labelId), std::move(onValueChanged));
	case Variant::Type::EulerAngles:
		return new InspectorPropertyEuler(std::move(info), node, std::move(labelId), std::move(onValueChanged));
	default:
		return nullptr;
	}
}
