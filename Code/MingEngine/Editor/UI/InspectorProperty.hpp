#pragma once

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/Variant.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"

#include <functional>
#include <string>
#include <vector>

class VBoxContainer;
class Label;
class Button;
class SpinBox;
class OptionButton;
class ColorRect;
struct EditorUIContext;

class InspectorProperty
{
public:
	using ValueChangedCallback = std::function<void(Variant const&)>;

public:
	virtual ~InspectorProperty();
	Control*                  GetControl() const;
	Variant::Type             GetType() const { return m_info.m_type; }
	std::string               GetDisplayName() const;
	void                      SetValueReader(std::function<Variant()> reader) { m_reader = std::move(reader); }
	void                      SetEditedObject(ObjectID id) { m_editedObject = id; }
	void                      UpdateValue(Variant const& value);
	void                      SetRevertValue(Variant const& value);
	void                      EndEditing(LineEdit::EditEndReason reason = LineEdit::EditEndReason::Interrupted);
	bool                      CanAssignResource(VirtualPath const& path) const;
	bool                      AssignResource(VirtualPath const& path);
	bool                      Contains(Vector2 localPosition) const;
	static InspectorProperty* Create(PropertyInfo info, std::string labelId, ValueChangedCallback onValueChanged);

protected:
	InspectorProperty(PropertyInfo info, std::string labelId, ValueChangedCallback onValueChanged);
	float          GetEditorDimension(char const* name, float fallback) const;
	void           BuildNumeric();
	void           BuildComponents();
	void           BuildText(bool resource = false);
	void           BuildScript();
	void           ShowScriptMenu();
	void           BuildBool();
	void           BuildEnum();
	void           EmitValueChanged(Variant const& value);
	void           SetError(std::string const& message);
	Variant        ReadValue() const;
	Variant        ReadComponent(int index) const;
	static Variant GetComponentValue(Variant const& value, int index);
	void           SetComponent(int index, Variant const& value);

protected:
	PropertyInfo             m_info;
	std::string              m_labelId;
	ValueChangedCallback     m_onValueChanged;
	std::function<Variant()> m_reader;
	Variant                  m_value;
	String                   m_editStartText;
	ObjectID                 m_rootID = ObjectID::Invalid;
	VBoxContainer*           m_root   = nullptr;
	VBoxContainer*           m_fields = nullptr;
	Label*                   m_error  = nullptr;
	Label*                   m_title  = nullptr;
	Button*                  m_revert = nullptr;
	Variant                  m_revertValue;
	LineEdit*                m_text    = nullptr;
	Button*                  m_button  = nullptr;
	OptionButton*            m_options = nullptr;
	ColorRect*               m_color   = nullptr;
	std::vector<SpinBox*>    m_spins;
	ObjectID m_editedObject = ObjectID::Invalid;
	Button* m_resourceButton = nullptr;
	Button* m_resourceMenu = nullptr;
	ObjectID m_tooltipID = ObjectID::Invalid;
	Label* m_resourceHint = nullptr;
	bool m_dropHighlighted = false;
};
