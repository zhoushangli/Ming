#include "MingEngine/Editor/UI/Inspector/InspectorProperty.hpp"

#include "MingEngine/Editor/UI/Inspector/InspectorPropertyBool.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorPropertyColor.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorPropertyEuler.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorPropertyFloat.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorPropertyInt.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorPropertyResource.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorPropertyString.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorPropertyVec3.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorPropertyVec2.hpp"
#include "MingEngine/Editor/UI/Inspector/InspectorPropertyEnum.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Editor/UI/Controls/EditorSpinSlider.hpp"
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
	auto* labelArea = MemNew<HBoxContainer>();
	labelArea->SetName("PropertyLabelArea");
	labelArea->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	labelArea->SetSeparation(0.f);
	labelArea->AddThemeChangedCallback(
		[control = labelArea, vector]()
		{
			control->SetCustomMinimumSize(
				Vector2(vector ? 0.f : static_cast<float>(control->GetThemeConstant("property_label_width", "Editor")), 0.f));
		});
	title->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	title->SetClipText(true);
	title->SetVerticalAlignment(VerticalAlignment::Center);
	row->AddNode(labelArea);
	labelArea->AddNode(title);
	m_fields = MemNew<VBoxContainer>();
	m_fields->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	m_fields->AddThemeChangedCallback(
		[control = m_fields]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")) * 0.5f);
		});
	auto* valueArea = MemNew<PanelContainer>();
	valueArea->SetName("PropertyValueArea");
	valueArea->SetThemeTypeVariation("EditorPropertyValue");
	valueArea->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	valueArea->AddThemeChangedCallback(
		[control = valueArea, vector]()
		{
			control->SetCustomMinimumSize(
				Vector2(vector ? 0.f : static_cast<float>(control->GetThemeConstant("property_label_width", "Editor")), 0.f));
		});
	valueArea->AddNode(m_fields);
	if (vector)
	{
		m_root->AddNode(valueArea);
	}
	else
	{
		row->AddNode(valueArea);
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
	labelArea->AddNode(revertSlot);
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

	for (EditorSpinSlider* spin : m_spins)
	{
		spin->SetValueChangedCallback({});
		spin->SetValueReader({});
		spin->SetDragCallbacks({}, {}, {});
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
	auto callback = m_numericDragging && m_editPreview ? m_editPreview : m_onValueChanged;
	if (value.GetType() == m_info.m_type && callback)
	{
		callback(value);
	}
}

void InspectorProperty::BindNumericDrag(EditorSpinSlider* spin)
{
	spin->SetDragCallbacks([this]()
	{
		m_numericStart = ReadValue();
		m_numericDragging = true;
		auto callback = m_editStarted;
		if (callback)
		{
			callback();
		}
	}, [this, spin](Variant const& value)
	{
		if (!m_numericDragging)
		{
			return;
		}

		if (m_info.m_type == Variant::Type::Int || m_info.m_type == Variant::Type::Float)
		{
			EmitValueChanged(value);
		}
		else
		{
			for (size_t i = 0; i < m_spins.size(); ++i)
			{
				if (m_spins[i] == spin)
				{
					SetComponent(static_cast<int>(i), value);
					break;
				}
			}
		}
	}, [this](bool accept)
	{
		m_numericDragging = false;
		auto callback = m_editEnded;
		if (callback)
		{
			callback(accept);
		}
		else if (!accept)
		{
			EmitValueChanged(m_numericStart);
		}
	});
}

void InspectorProperty::BuildNumeric()
{
	auto* spin = MemNew<EditorSpinSlider>();
	spin->GetLineEdit()->SetThemeTypeVariation("EditorPropertyEdit");
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
	BindNumericDrag(spin);
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
		auto* spin = MemNew<EditorSpinSlider>();
		if (vector)
		{
			spin->GetLineEdit()->SetThemeTypeVariation("EditorVectorEdit");
		}
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
		BindNumericDrag(spin);
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
	m_button->SetThemeTypeVariation("EditorPropertyButton");
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
	m_options->SetThemeTypeVariation("EditorPropertyOption");
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


void InspectorProperty::BuildResource()
{
	m_root->SetHorizontalSizeFlags(Control::Size_ShrinkBegin);
	auto* row = MemNew<HBoxContainer>();
	row->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	row->SetSeparation(0.f);
	m_fields->AddNode(row);
	m_resourceButton = MemNew<Button>();
	m_resourceButton->SetThemeTypeVariation("EditorScriptButton");
	m_resourceButton->SetName(m_info.m_name == "script" ? "ScriptResource" : "ResourceValue");
	m_resourceButton->SetClipText(true);
	m_resourceButton->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	m_resourceButton->SetText("<empty>");
	m_resourceButton->SetPressedCallback([this]()
	{
		Ref<Resource> resource(ReadValue());
		if (resource.IsValid())
		{
			if (m_info.m_name == "script")
			{
				EditorNode::Get()->OpenCSharpScript(resource->GetVirtualPath());
			}
			else
			{
				auto& files = EditorNode::Get()->m_editorUI->GetPanel<FileSystemPanel>();
				files.NavigateTo(resource->GetVirtualPath().GetParent());
				files.SelectPath(resource->GetVirtualPath());
			}
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
	m_root->SetDragCallbacks({}, [this](Vector2 const&, GuiDragData const& data)
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
	load->SetVisible(m_info.m_name == "script");
	m_resourceMenu = MemNew<Button>();
	m_resourceMenu->SetThemeTypeVariation("EditorScriptMenu");
	m_resourceMenu->SetName(m_info.m_name == "script" ? "ScriptMenu" : "ResourceMenu");
	m_resourceMenu->SetIcon(m_root->GetThemeIcon("Dropdown", "EditorIcons"));
	m_resourceMenu->SetPressedCallback([this]() { ShowResourceMenu(); });
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

void InspectorProperty::ShowResourceMenu()
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
	if (m_info.m_name != "script")
	{
		items.erase(items.begin(), items.begin() + 2);
	}

	ui->ShowMenu(m_resourceMenu, m_resourceMenu->GetGlobalPosition() + Vector2(0.f, m_resourceMenu->GetSize().y), items);
}

void InspectorProperty::BuildText(bool resource)
{
	if (resource)
	{
		BuildResource();
		return;
	}

	m_text = MemNew<LineEdit>();
	m_text->SetMinimumCharacterWidth(1);
	m_text->SetThemeTypeVariation("EditorPropertyEdit");
	m_fields->AddNode(m_text);
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
		String label("<empty>");
		if (resource.IsValid())
		{
			label = resource->GetVirtualPath().GetFileName();
			if (label.IsEmpty())
			{
				label = resource->GetName().IsEmpty() ? String(resource->GetClassName()) : resource->GetName();
			}
		}

		m_resourceButton->SetText(label);
		m_resourceButton->SetIcon(resource.IsValid() ? m_root->GetThemeIcon(resource->GetClassName(), "EditorIcons") : Ref<TextureResource>());
		if (EditorNode::Get() != nullptr && EditorNode::Get()->m_editorUI->GetRootControl() != nullptr)
		{
			auto& inspector = EditorNode::Get()->m_editorUI->GetPanel<InspectorPanel>();
			float const padding = m_root->GetThemeConstant("panel_margin", "Editor") * 2.f
				+ m_root->GetThemeConstant("scrollbar_width", "ScrollContainer");
			auto* row = static_cast<Control*>(m_title->GetParent()->GetParent());
			float const minimum = row->GetCombinedMinimumSize().x;
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

	for (EditorSpinSlider* spin : m_spins)
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

void InspectorProperty::SetContinuousEditCallbacks(
	std::function<void()> started, ValueChangedCallback preview, std::function<void(bool)> ended)
{
	m_editStarted = std::move(started);
	m_editPreview = std::move(preview);
	m_editEnded   = std::move(ended);
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
			auto callback = m_editStarted;
			if (callback)
			{
				callback();
			}
		});
	m_color->SetColorChangedCallback(
		[this](Color const& color)
		{
			auto callback = m_editPreview;
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
			auto callback = m_editEnded;
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
