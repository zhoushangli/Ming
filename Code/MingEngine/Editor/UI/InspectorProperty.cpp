#include "MingEngine/Editor/UI/InspectorProperty.hpp"

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
#include "MingEngine/Scene/GUI/ColorPickerButton.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include <cctype>
#include <cmath>

InspectorProperty::InspectorProperty(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
	: m_info(std::move(info)), m_labelId(std::move(labelId)), m_onValueChanged(std::move(changed))
{
	m_root   = MemNew<VBoxContainer>();
	m_rootID = m_root->GetObjectID();
	m_root->SetName(m_info.m_name);
	bool const vector = m_info.m_type == Variant::Type::Vector3 || m_info.m_type == Variant::Type::EulerAngles;
	if (EditorNode::Get() != nullptr && EditorNode::Get()->m_editorUI->GetRootControl() != nullptr)
	{
		m_root->SetThemeSource(EditorNode::Get()->m_editorUI->GetRootControl());
	}
	m_root->SetSerializable(false);
	m_root->AddThemeChangedCallback(
		[control = m_root]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("status_spacing", "Editor")) * 0.25f);
		});
	auto* title = MemNew<Label>();
	m_title     = title;
	title->SetText(GetDisplayName());
	title->SetThemeTypeVariation("EditorPropertyTitle");
	auto* row = MemNew<HBoxContainer>();
	row->AddThemeChangedCallback(
		[control = row]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("status_spacing", "Editor")) * 0.5f);
		});
	m_root->AddNode(row);
	title->AddThemeChangedCallback(
		[control = title, vector]()
		{
			control->SetCustomMinimumSize(
				Vector2(vector ? 0.f : static_cast<float>(control->GetThemeConstant("property_label_width", "Editor")), 0.f));
		});
	if (vector)
	{
		title->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	}
	title->SetClipText(true);
	title->SetVerticalAlignment(VerticalAlignment::Center);
	row->AddNode(title);
	m_fields = MemNew<VBoxContainer>();
	m_fields->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	m_fields->AddThemeChangedCallback(
		[control = m_fields]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")) * 0.5f);
		});
	if (vector)
	{
		m_root->AddNode(m_fields);
	}
	else
	{
		row->AddNode(m_fields);
	}
	m_revert = MemNew<Button>();
	m_revert->SetName("RevertProperty");
	m_revert->SetThemeTypeVariation("EditorPanelAction");
	m_revert->SetIcon(EditorIcons::GetIconTexture("Undo"));
	m_revert->AddThemeChangedCallback(
		[control = m_revert]()
		{
			control->SetCustomMinimumSize(Vector2(
				static_cast<float>(control->GetThemeConstant("action_slot", "Editor")),
				static_cast<float>(control->GetThemeConstant("action_slot", "Editor"))));
		});
	m_revert->SetDisabled(true);
	m_revert->SetVisible(false);
	m_revert->SetPressedCallback(
		[this]()
		{
			EmitValueChanged(m_revertValue);
		});
	auto* revertSlot = MemNew<HBoxContainer>();
	revertSlot->AddThemeChangedCallback(
		[control = revertSlot]()
		{
			control->SetCustomMinimumSize(
				Vector2(static_cast<float>(control->GetThemeConstant("action_slot", "Editor")), 0.f));
		});
	row->AddNode(revertSlot);
	revertSlot->AddNode(m_revert);
	m_error = MemNew<Label>();
	m_error->SetThemeTypeVariation("EditorErrorLabel");
	m_error->SetVisible(false);
	m_root->AddNode(m_error);
}

InspectorProperty::~InspectorProperty()
{
	if (auto* tooltip = ObjectDatabase::GetInstance<Control>(m_tooltipID))
	{
		tooltip->QueueFree();
	}

	if (GetControl() != nullptr && m_color != nullptr)
	{
		m_color->ClosePicker(false, false);
	}

	m_onValueChanged = {};
	m_reader         = {};
	if (GetControl() == nullptr)
	{
		return;
	}

	EndEditing();
	if (m_color != nullptr)
	{
		m_color->SetColorChangedCallback({});
		m_color->SetEditingStartedCallback({});
		m_color->SetEditingEndedCallback({});
	}

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
	spin->SetStepButtonsVisible(false);
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
	m_fields->AddNode(spin);
	m_spins.push_back(spin);
}

void InspectorProperty::BuildComponents()
{
	bool const vector = m_info.m_type == Variant::Type::Vector3 || m_info.m_type == Variant::Type::EulerAngles;
	int const  count  = m_info.m_type == Variant::Type::Vector2 ? 2 : 3;

	auto* components = MemNew<HBoxContainer>();
	components->AddThemeChangedCallback(
		[control = components]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")) * 0.5f);
		});
	m_fields->AddNode(components);
	char const* names[]  = { "X", "Y", "Z", "W" };
	char const* axes[] = { "x", "y", "z" };
	char const* axisColors[] = { "property_color_x", "property_color_y", "property_color_z" };
	for (int i = 0; i < count; ++i)
	{
		auto* row   = MemNew<HBoxContainer>();
		if (vector)
		{
			auto* panel = MemNew<PanelContainer>();
			panel->SetName(axes[i]);
			panel->SetThemeTypeVariation("EditorVectorField");
			panel->SetHorizontalSizeFlags(Control::Size_ExpandFill);
			components->AddNode(panel);
			panel->AddNode(row);
		}
		else
		{
			components->AddNode(row);
		}

		auto* label = MemNew<Label>();
		label->SetText(String(vector ? axes[i] : names[i]));
		label->SetThemeTypeVariation("EditorPropertyLabel");
		label->AddThemeChangedCallback(
			[control = label, vector, color = vector ? axisColors[i] : ""]()
			{
				control->SetCustomMinimumSize(
					Vector2(static_cast<float>(control->GetThemeConstant(vector ? "icon_size" : "spacing", vector ? "EditorGroupButton" : "Editor")), 0.f));
				if (vector)
				{
					control->SetFontColor(control->GetThemeColor(color, "Editor"));
				}
			});
		label->SetClipText(true);
		label->SetVerticalAlignment(VerticalAlignment::Center);
		row->AddThemeChangedCallback(
			[control = row]()
			{
				control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")) * 0.25f);
			});
		row->AddNode(label);
		auto* spin = MemNew<SpinBox>();
		if (vector)
		{
			spin->GetLineEdit()->SetThemeTypeVariation("EditorVectorEdit");
		}
		spin->SetStepButtonsVisible(false);
		spin->SetHorizontalSizeFlags(Control::Size_ExpandFill);
		spin->Configure(
			false,
			m_info.m_hint == PropertyInfo::Hint::Range ? (m_info.m_hintData.empty() ? "," : m_info.m_hintData) : "");
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

		if (m_info.m_type == Variant::Type::EulerAngles)
		{
			auto* suffix = MemNew<Label>();
			suffix->SetText(U"\u00b0");
			suffix->SetVerticalAlignment(VerticalAlignment::Center);
			row->AddNode(suffix);
		}
		row->SetHorizontalSizeFlags(Control::Size_ExpandFill);
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
	m_fields->AddNode(m_button);
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
	m_fields->AddNode(m_options);
}


void InspectorProperty::BuildScript()
{
	m_root->SetHorizontalSizeFlags(Control::Size_ShrinkBegin);
	if (auto* slot = dynamic_cast<Control*>(m_revert->GetParent()))
	{
		slot->SetVisible(false);
	}

	m_title->AddThemeChangedCallback([label = m_title]()
	{
		label->SetCustomMinimumSize(Vector2(static_cast<float>(label->GetThemeConstant("script_label_width", "Editor")), 0.f));
	});

	auto* row = MemNew<HBoxContainer>();
	row->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	row->SetSeparation(0.f);
	m_fields->AddNode(row);
	m_resourceButton = MemNew<Button>();
	m_resourceButton->SetThemeTypeVariation("EditorScriptButton");
	m_resourceButton->SetName("ScriptResource");
	m_resourceButton->SetClipText(true);
	m_resourceButton->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	m_resourceButton->AddThemeChangedCallback([button = m_resourceButton]()
	{
		button->SetCustomMinimumSize(Vector2(static_cast<float>(button->GetThemeConstant("property_label_width", "Editor")), 0.f));
	});
	m_resourceButton->SetText("<empty>");
	m_resourceButton->SetPressedCallback([this]()
	{
		Ref<Resource> resource(ReadValue());
		if (resource.IsValid())
		{
			EditorNode::Get()->OpenCSharpScript(resource->GetVirtualPath());
		}
	});
	row->AddNode(m_resourceButton);
	m_resourceButton->SetDragCallbacks({}, [this](Vector2 const&, GuiDragData const& data)
	{
		VirtualPath path;
		return data.m_type == String("file") && VirtualPath::TryParse(data.m_value.ToUtf8(), path) && CanAssignResource(path);
	}, [this](Vector2 const&, GuiDragData const& data)
	{
		VirtualPath path;
		if (VirtualPath::TryParse(data.m_value.ToUtf8(), path))
		{
			AssignResource(path);
		}
	});
	Button* load = MemNew<Button>();
	load->SetThemeTypeVariation("EditorScriptAction");
	load->SetName("LoadScript");
	load->SetIcon(m_root->GetThemeIcon("Folder", "EditorIcons"));
	load->SetPressedCallback([this]()
	{
		auto changed = m_onValueChanged;
		ObjectID const ownerID = m_editedObject;
		EditorNode::Get()->m_editorUI->OpenScriptFile([changed, ownerID](VirtualPath const& path)
		{
			Node* owner = ObjectDatabase::GetInstance<Node>(ownerID);
			if (owner == nullptr || owner->IsQueuedForDeletion())
			{
				return;
			}

			std::string const base = g_engine->m_scriptSystem->GetScriptNativeBase(path);
			if (g_engine->m_scriptSystem->IsProjectAssemblyCurrent() && !base.empty()
				&& !ClassDatabase::IsSubclassOf(owner->GetClassName(), base))
			{
				EditorNode::Get()->m_editorUI->Warning("Script", "The script requires " + base + ".");
				return;
			}

			Ref<Resource> resource = ResourceLoader::Load(path);
			if (resource.IsValid() && changed)
			{
				changed(Variant(resource));
			}
		});
	});
	row->AddNode(load);
	m_resourceMenu = MemNew<Button>();
	m_resourceMenu->SetThemeTypeVariation("EditorScriptMenu");
	m_resourceMenu->SetName("ScriptMenu");
	m_resourceMenu->SetIcon(m_root->GetThemeIcon("Dropdown", "EditorIcons"));
	m_resourceMenu->SetPressedCallback([this]() { ShowScriptMenu(); });
	row->AddNode(m_resourceMenu);
	if (EditorNode::Get() != nullptr && EditorNode::Get()->m_editorUI->GetRootControl() != nullptr)
	{
		auto* tooltip = MemNew<PanelContainer>();
		m_tooltipID = tooltip->GetObjectID();
		tooltip->SetThemeTypeVariation("EditorMenuPopup");
		tooltip->SetMouseFilter(Control::MouseFilter::Ignore);
		tooltip->SetSerializable(false);
		tooltip->SetVisible(false);
		EditorNode::Get()->m_editorUI->GetRootControl()->GetViewport()->AddNode(tooltip);
		tooltip->SetThemeSource(m_root);
		m_resourceHint = MemNew<Label>();
		tooltip->AddNode(m_resourceHint);
	}

}

void InspectorProperty::ShowScriptMenu()
{
	EditorUI* ui = EditorNode::Get()->m_editorUI;
	Ref<Resource> resource(ReadValue());
	auto changed = m_onValueChanged;
	ObjectID const ownerID = m_editedObject;
	std::vector<EditorUI::MenuItem> items = {
		{ "New Script...", [ui, ownerID, changed]()
			{
				Node* owner = ObjectDatabase::GetInstance<Node>(ownerID);
				if (owner == nullptr || owner->IsQueuedForDeletion())
				{
					return;
				}

				ui->CreateScript(ui->GetPanel<FileSystemPanel>().GetDirectory(), owner->GetClassName(),
					[ui, ownerID, changed](VirtualPath const& path)
					{
						Node* target = ObjectDatabase::GetInstance<Node>(ownerID);
						if (target != nullptr && !target->IsQueuedForDeletion() && changed)
						{
							Ref<Resource> script = ResourceLoader::Load(path);
							changed(Variant(script));
						}

						ui->GetPanel<FileSystemPanel>().NavigateTo(path.GetParent());
						ui->GetPanel<FileSystemPanel>().SelectPath(path);
					});
			} },
		{ "Open", [resource]()
			{
				if (resource.IsValid())
				{
					EditorNode::Get()->OpenCSharpScript(resource->GetVirtualPath());
				}
			} },
		{ "Clear", [changed]()
			{
				if (changed)
				{
					changed(Variant(static_cast<Object*>(nullptr)));
				}
			} }
	};
	items[1].m_enabled = resource.IsValid();
	items[2].m_enabled = resource.IsValid();
	ui->ShowMenu(m_resourceMenu, m_resourceMenu->GetGlobalPosition() + Vector2(0.f, m_resourceMenu->GetSize().y), items);
}

void InspectorProperty::BuildText(bool resource)
{
	if (resource && m_info.m_name == "script")
	{
		BuildScript();
		return;
	}

	m_text = MemNew<LineEdit>();
	m_text->SetMinimumCharacterWidth(1);
	m_text->SetEditable(!resource);
	m_fields->AddNode(m_text);
	if (resource)
	{
		auto check = [this](Vector2 const&, GuiDragData const& data)
		{
			VirtualPath path;
			return data.m_type == String("file") && VirtualPath::TryParse(data.m_value.ToUtf8(), path)
				   && CanAssignResource(path);
		};
		auto drop = [this](Vector2 const&, GuiDragData const& data)
		{
			VirtualPath path;
			if (data.m_type == String("file") && VirtualPath::TryParse(data.m_value.ToUtf8(), path))
			{
				AssignResource(path);
			}
		};
		m_text->SetDragCallbacks({}, check, drop);
		m_root->SetDragCallbacks({}, check, drop);
		m_button = MemNew<Button>();
		m_button->SetText("Clear");
		m_button->SetPressedCallback(
			[this]()
			{
				EmitValueChanged(Variant(static_cast<Object*>(nullptr)));
			});
		m_fields->AddNode(m_button);
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

void InspectorProperty::SetRevertValue(Variant const& value)
{
	m_revertValue = value;
	m_revert->SetDisabled(value.GetType() != m_info.m_type);
	UpdateValue(m_value);
}

void InspectorProperty::UpdateValue(Variant const& value)
{
	if (value.GetType() != m_info.m_type || GetControl() == nullptr)
	{
		return;
	}

	m_value = value;
	if (m_resourceButton != nullptr)
	{
		Ref<Resource> resource(value);
		m_resourceButton->SetText(resource.IsValid() ? String(resource->GetVirtualPath().GetFileName()) : String("<empty>"));
		m_resourceButton->SetIcon(resource.IsValid() ? m_root->GetThemeIcon("CSharpScript", "EditorIcons") : Ref<TextureResource>());
		if (EditorNode::Get() != nullptr && EditorNode::Get()->m_editorUI->GetRootControl() != nullptr)
		{
			auto& inspector = EditorNode::Get()->m_editorUI->GetPanel<InspectorPanel>();
			auto* outer = static_cast<HBoxContainer*>(m_title->GetParent());
			auto* buttons = static_cast<HBoxContainer*>(m_resourceButton->GetParent());
			float const padding = static_cast<float>(m_root->GetThemeConstant("panel_margin", "Editor")) * 2.f
				+ static_cast<float>(m_root->GetThemeConstant("scrollbar_width", "ScrollContainer"));
			float const minimum = m_title->GetCombinedMinimumSize().x + outer->GetSeparation() + buttons->GetCombinedMinimumSize().x;
			inspector.SetCustomMinimumSize(Vector2(minimum + padding, 0.f));
			m_root->SetCustomMinimumSize(Vector2(std::max(minimum, inspector.GetSize().x - padding), 0.f));
		}

		Viewport* viewport = m_root->GetViewport();
		VirtualPath path;
		bool const highlighted = viewport != nullptr && viewport->IsGuiDragging()
			&& viewport->GetGuiDragData().m_type == String("file")
			&& VirtualPath::TryParse(viewport->GetGuiDragData().m_value.ToUtf8(), path) && CanAssignResource(path);
		if (highlighted != m_dropHighlighted)
		{
			m_dropHighlighted = highlighted;
			for (char const* state : { "normal", "hover", "pressed", "hover_pressed" })
			{
				if (highlighted)
				{
					m_resourceButton->AddThemeStyleBoxOverride(state, m_resourceButton->GetThemeStyleBox("hover", "EditorScriptButton"));
				}
				else
				{
					m_resourceButton->RemoveThemeStyleBoxOverride(state);
				}
			}
		}

		if (auto* tooltip = ObjectDatabase::GetInstance<Control>(m_tooltipID))
		{
			bool const show = resource.IsValid() && m_resourceButton->IsHovered() && viewport != nullptr
				&& !viewport->IsGuiDragging() && viewport->GetGuiPopup() == nullptr;
			tooltip->SetVisible(show);
			if (show)
			{
				m_resourceHint->SetText(resource->GetVirtualPath().GetString());
				Vector2 const size = tooltip->GetCombinedMinimumSize();
				Vector2 const bounds(viewport->GetOutputResolution());
				Vector2 const origin = m_resourceButton->GetGlobalPosition() + Vector2(0.f, m_resourceButton->GetSize().y);
				tooltip->SetPosition(Vector2(std::max(0.f, std::min(origin.x, bounds.x - size.x)),
					std::max(0.f, std::min(origin.y, bounds.y - size.y))));
				tooltip->UpdateSize(size);
			}
		}
	}
	m_revert->SetVisible(m_revertValue.GetType() == value.GetType() && value != m_revertValue);
	for (int i = 0; i < static_cast<int>(m_spins.size()); ++i)
	{
		m_spins[i]->SetValueNoSignal(
			m_info.m_type == Variant::Type::Int || m_info.m_type == Variant::Type::Float ? value
																						 : GetComponentValue(value, i));
	}

	if (m_color != nullptr)
	{
		m_color->SetColorNoSignal(value.As<Color>());
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

	if (m_color != nullptr)
	{
		m_color->ClosePicker(false, false);
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
	if (!resource.IsValid() || !ClassDatabase::IsSubclassOf(resource->GetClassName(), m_info.m_hintData))
	{
		return false;
	}

	if (m_info.m_name == "script" && g_engine->m_scriptSystem != nullptr && g_engine->m_scriptSystem->IsProjectAssemblyCurrent())
	{
		std::string const base = g_engine->m_scriptSystem->GetScriptNativeBase(path);
		Node* owner = ObjectDatabase::GetInstance<Node>(m_editedObject);
		if (!base.empty() && owner != nullptr && !ClassDatabase::IsSubclassOf(owner->GetClassName(), base))
		{
			return false;
		}
	}

	return true;
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

float InspectorProperty::GetEditorDimension(char const* name, float fallback) const
{
	int const value = m_root->GetThemeConstant(name, "Editor");
	return value > 0 ? static_cast<float>(value) : fallback;
}

void InspectorProperty::SetColorEditCallbacks(
	std::function<void()> started, ValueChangedCallback preview, std::function<void(bool)> ended)
{
	m_colorStarted = std::move(started);
	m_colorPreview = std::move(preview);
	m_colorEnded   = std::move(ended);
}

void InspectorProperty::BuildColor()
{
	m_color = MemNew<ColorPickerButton>();
	m_color->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	m_color->AddThemeChangedCallback(
		[button = m_color]()
		{
			button->SetCustomMinimumSize(
				Vector2(0.f, static_cast<float>(button->GetThemeConstant("color_height", "Editor"))));
		});
	m_color->SetEditingStartedCallback(
		[this]()
		{
			m_colorStart  = ReadValue();
			auto callback = m_colorStarted;
			if (callback)
			{
				callback();
			}
		});
	m_color->SetColorChangedCallback(
		[this](Color const& color)
		{
			auto callback = m_colorPreview;
			if (callback)
			{
				callback(Variant(color));
			}
			else
			{
				EmitValueChanged(Variant(color));
			}
		});
	m_color->SetEditingEndedCallback(
		[this](bool accept)
		{
			auto callback = m_colorEnded;
			if (callback)
			{
				callback(accept);
			}
			else if (!accept && m_colorStart.Is<Color>())
			{
				EmitValueChanged(m_colorStart);
			}
		});
	m_color->SetScreenColorReader(
		[](Vector2 const& position, Color& color)
		{
			return g_engine != nullptr && g_engine->m_windowSystem != nullptr
				   && g_engine->m_windowSystem->TryGetScreenColor(position, color);
		});
	m_fields->AddNode(m_color);
}
