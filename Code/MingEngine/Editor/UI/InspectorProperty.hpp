#pragma once

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

#include <functional>
#include <string>

class Node;
struct EditorUIContext;

class InspectorProperty
{
public:
	using ValueChangedCallback = std::function<void(Variant const&)>;

	virtual ~InspectorProperty() = default;

	void Render(EditorUIContext& context);

	Variant::Type GetType() const;
	char const* GetLabelId() const;
	std::string GetDisplayName() const;

	static InspectorProperty* Create(
		PropertyInfo info,
		Node* node,
		std::string labelId,
		ValueChangedCallback onValueChanged);

protected:
	InspectorProperty(PropertyInfo info, Node* node, std::string labelId, ValueChangedCallback onValueChanged);

	virtual void RenderValue(EditorUIContext& context, Variant const& value) = 0;
	void EmitValueChanged(Variant const& value) const;
	Variant GetCurrentValue() const;

protected:
	PropertyInfo m_info;
	Node* m_node = nullptr;
	std::string m_labelId;
	ValueChangedCallback m_onValueChanged;
};
