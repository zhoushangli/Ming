#pragma once

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

#include <string>

class Node;

class InspectorProperty
{
public:
	InspectorProperty(PropertyInfo info, Variant value, std::string labelId);
	virtual ~InspectorProperty() = default;

	// Each subclass only needs to override this
	virtual void Render() = 0;

	// ——— Provided by base class ———
	bool WasEdited() const;
	void Apply(Node* node);
	Variant::Type GetType() const;
	char const* GetLabelId() const;
	std::string GetDisplayName() const;

	static InspectorProperty* Create(PropertyInfo info, Variant value, std::string labelId);

protected:
	PropertyInfo m_info;
	Variant m_value;
	std::string m_labelId;
	bool m_edited = false;
};

