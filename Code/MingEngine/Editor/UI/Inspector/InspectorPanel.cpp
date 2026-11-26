#include "MingEngine/Editor/UI/Inspector/InspectorPanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Editor/EditorChecker.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Core/IO/ResourceSaver.hpp"
#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"
#include "MingEngine/Scene/3D/MeshInstance3D.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/Resource/SurfaceTool.hpp"
#include <cmath>
#include <algorithm>
#include <cctype>

InspectorPanel::InspectorPanel() : EditorPanel("Inspector")
{
}

InspectorPanel::~InspectorPanel()
{
	ClearProperties();
}

void InspectorPanel::ClearProperties()
{
	EndPropertyEdit(false);
	if (GetViewport() != nullptr)
	{
		GetViewport()->CancelGuiInteraction(this);
	}
	for (PropertyGroup& group : m_propertyGroups)
	{
		for (PropertyEntry& entry : group.m_properties)
		{
			delete entry.m_property;
		}
	}

	m_propertyGroups.clear();
	EditorPopupUtils::ClearContent(m_content);
}

void InspectorPanel::BuildInheritanceChain(std::string const& className)
{
	m_inheritanceChain.clear();
	std::string current = className;
	while (true)
	{
		m_inheritanceChain.push_back(current);
		ClassInfo const* info = ClassDatabase::GetClassInfo(current);
		if (current == "Node" || current == "Resource" || info == nullptr || info->m_parentClassName.empty()
			|| info->m_parentClassName == current)
		{
			break;
		}

		current = info->m_parentClassName;
	}
}

void InspectorPanel::RefreshValues()
{
	if (m_propertyEdit.m_target.IsValid() && !m_propertyEdit.m_applying)
	{
		Object* target = GetTarget(m_propertyEdit.m_target);
		if (target == nullptr
			|| m_propertyEdit.m_getter->Invoke(target, {}) != m_propertyEdit.m_last)
		{
			EndPropertyEdit(false);
		}
	}

	Object* node = GetTarget(m_cachedObjectID);
	if (node == nullptr)
	{
		return;
	}

	for (PropertyGroup& group : m_propertyGroups)
	{
		for (PropertyEntry& entry : group.m_properties)
		{
			if (entry.m_getter != nullptr)
			{
				entry.m_property->UpdateValue(entry.m_getter->Invoke(node, {}));
			}
		}
	}
}

void InspectorPanel::CommitValue(
	std::string const& propertyName,
	ObjectID           id,
	MethodBind const*  setter,
	MethodBind const*  getter,
	Variant const&     value,
	Variant const*     original)
{
	Object* node = GetTarget(id);
	if (id != m_cachedObjectID || node == nullptr
		|| setter == nullptr || getter == nullptr)
	{
		return;
	}

	Variant const oldValue = original != nullptr ? *original : getter->Invoke(node, {});
	if (value.GetType() != oldValue.GetType() || value == oldValue)
	{
		return;
	}

	node = GetTarget(id);
	if (node == nullptr)
	{
		return;
	}

	Ref<Resource> oldResource(oldValue);
	if (original == nullptr)
	{
		setter->Invoke(node, { value });
	}
	node = GetTarget(id);
	if (node == nullptr)
	{
		return;
	}

	Variant const actual = getter->Invoke(node, {});
	if (actual != oldValue && EditorNode::Get() != nullptr)
	{
		auto* editor  = EditorNode::Get();
		auto& history = editor->GetUndoRedo();
		if (history.CreateAction("Edit " + propertyName, GetUndoHistory()))
		{
			history.AddDoProperty(node, propertyName, actual);
			history.AddUndoProperty(node, propertyName, oldValue);
			if (dynamic_cast<Resource*>(node) != nullptr)
			{
				history.AddDoMethod(editor, "MarkResourceDirty", { Variant(node) });
				history.AddUndoMethod(editor, "MarkResourceDirty", { Variant(node) });
			}
			else
			{
				history.AddDoMethod(editor, "MarkSceneDirty");
				history.AddUndoMethod(editor, "MarkSceneDirty");
			}
			history.CommitAction(false);
		}
		if (dynamic_cast<Resource*>(node) != nullptr)
		{
			editor->MarkResourceDirty(node);
		}
		else
		{
			editor->MarkSceneDirty();
		}
	}

	RefreshValues();
}

void InspectorPanel::RebuildProperties(EditorUIContext&)
{
	Object* node = GetTarget(m_cachedObjectID);
	if (node == nullptr)
	{
		return;
	}

	auto const defaults = ClassDatabase::GetPropertyDefaultValues(node->GetClassName());
	for (std::string const& className : m_inheritanceChain)
	{
		PropertyGroup group;
		group.m_className = className;
		auto* section     = MemNew<Control>();
		section->AddThemeChangedCallback(
			[control = section]()
			{
				control->SetCustomMinimumSize(
					Vector2(0.f, static_cast<float>(control->GetThemeConstant("section_height", "Editor"))));
			});
		m_content->AddNode(section);
		group.m_header = section;

		auto* header = MemNew<PanelContainer>();
		header->SetThemeTypeVariation("EditorInspectorClassHeader");
		header->AddThemeChangedCallback(
			[control = header]()
			{
				control->SetCustomMinimumSize(
					Vector2(0.f, static_cast<float>(control->GetThemeConstant("section_height", "Editor"))));
			});
		header->SetClipContents(true);
		section->AddNode(header);
		group.m_bar = header;

		auto* heading = MemNew<HBoxContainer>();
		heading->AddThemeChangedCallback(
			[control = heading]()
			{
				control->SetSeparation(static_cast<float>(control->GetThemeConstant("spacing", "Editor")));
			});
		heading->SetHorizontalSizeFlags(Size_ShrinkCenter);
		heading->SetVerticalSizeFlags(Size_ShrinkCenter);
		header->AddNode(heading);

		auto* icon = MemNew<TextureRect>();
		icon->SetTexture(GetThemeIcon(className, "EditorIcons"));
		icon->SetTint(EditorIcons::GetIconColor(className, this));
		group.m_icon = icon;
		icon->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
		icon->SetStretchMode(TextureRect::StretchMode::KeepAspectCentered);
		icon->AddThemeChangedCallback(
			[control = icon]()
			{
				control->SetCustomMinimumSize(Vector2(
					static_cast<float>(control->GetThemeConstant("icon_size", "Button")),
					static_cast<float>(control->GetThemeConstant("icon_size", "Button"))));
			});
		heading->AddNode(icon);

		auto* title = MemNew<Label>();
		title->SetText(className);
		title->SetThemeTypeVariation("EditorInspectorClassLabel");
		title->SetTextOverrunBehavior(TextOverrunBehavior::TrimCharacters);
		title->SetVerticalAlignment(VerticalAlignment::Center);
		heading->AddNode(title);
		group.m_title = title;

		std::string sectionName;
		std::string sectionPrefix;
		int         sectionIndex = -1;
		auto properties = ClassDatabase::GetProperties(className);
		if (className == "Node")
		{
			if (auto* script = ClassDatabase::FindProperty("Object", "script"))
			{
				properties.emplace_back(
					Variant::Type::Empty, "", PropertyInfo::Hint::None, "", PropertyInfo::UsageFlags::Group);
				properties.push_back(*script);
			}
		}

		for (PropertyInfo const& prop : properties)
		{
			if (prop.HasUsage(PropertyInfo::UsageFlags::Group))
			{
				sectionName  = prop.m_name;
				sectionPrefix = prop.m_hintData;
				sectionIndex = -1;
				continue;
			}

			if (!prop.HasUsage(PropertyInfo::UsageFlags::Inspector))
			{
				continue;
			}

			ObjectID const    id     = m_cachedObjectID;
			MethodBind const* setter = prop.GetSetter();
			MethodBind const* getter = prop.GetGetter();
			auto*             field  = InspectorProperty::Create(
				prop,
				className + "::" + prop.m_name,
				[this, id, setter, getter, name = prop.m_name](Variant const& value)
				{
					CommitValue(name, id, setter, getter, value);
				});
			if (field == nullptr)
			{
				continue;
			}
			field->SetDisplayPrefix(sectionPrefix);

			if (prop.m_type == Variant::Type::Color || prop.m_type == Variant::Type::Int
				|| prop.m_type == Variant::Type::Float || prop.m_type == Variant::Type::Vector2
				|| prop.m_type == Variant::Type::Vector3 || prop.m_type == Variant::Type::EulerAngles)
			{
				field->SetContinuousEditCallbacks(
					[this, id, setter, getter, field, name = prop.m_name]()
					{
						BeginPropertyEdit(name, id, setter, getter, field);
					},
					[this](Variant const& value)
					{
						PreviewPropertyValue(value);
					},
					[this](bool accept)
					{
						EndPropertyEdit(accept);
					});
			}

			if (!sectionName.empty() && sectionIndex < 0)
			{
				sectionIndex = static_cast<int>(group.m_sections.size());
				auto* sectionRoot = MemNew<VBoxContainer>();
				sectionRoot->SetName("InspectorGroup_" + sectionName);
				m_content->AddNode(sectionRoot);
				Button* sectionHeading = EditorPopupUtils::AddButton(
					sectionRoot,
					sectionName,
					[this, className, sectionIndex]()
					{
						for (PropertyGroup& current : m_propertyGroups)
						{
							if (current.m_className == className)
							{
								auto& section = current.m_sections[sectionIndex];
								section.m_collapsed = !section.m_collapsed;
								break;
							}
						}

						FilterProperties();
					});
				sectionHeading->SetThemeTypeVariation("EditorGroupButton");
				sectionHeading->SetName("InspectorGroupHeading_" + sectionName);
				sectionHeading->SetHorizontalAlignment(HorizontalAlignment::Left);
				sectionHeading->SetClipText(true);
				sectionHeading->SetIcon(GetThemeIcon("ChevronDown", "EditorIcons"));
				auto* content = MemNew<MarginContainer>();
				sectionRoot->AddNode(content);
				content->AddThemeChangedCallback(
					[content, sectionHeading]()
					{
						auto style = sectionHeading->GetThemeStyleBox("normal");
						float const inset = (style.IsValid() ? style->GetContentMargin(ThemeStyleBox::Side::Left) : 0.f)
							+ sectionHeading->GetThemeConstant("icon_size", "EditorGroupButton")
							+ sectionHeading->GetThemeConstant("icon_separation", "EditorGroupButton");
						content->SetMargins(inset, 0.f, 0.f, 0.f);
					});
				auto* fields = MemNew<VBoxContainer>();
				content->AddNode(fields);
				fields->AddThemeChangedCallback(
					[fields]()
					{
						fields->SetSeparation(static_cast<float>(fields->GetThemeConstant("panel_margin", "Editor")));
					});
				group.m_sections.push_back({ sectionName, sectionRoot, sectionHeading, content, fields, false });
			}

			field->SetEditedObject(id);
			if (auto found = defaults.find(prop.m_name); found != defaults.end())
			{
				field->SetRevertValue(found->second);
			}

			field->SetValueReader(
				[this, id, getter]() -> Variant
				{
					Object* target = GetTarget(id);
					if (target == nullptr || getter == nullptr)
					{
						return {};
					}

					return getter->Invoke(target, {});
				});
			auto* destination = sectionIndex >= 0 ? group.m_sections[sectionIndex].m_fields : m_content;
			destination->AddNode(field->GetControl());
			group.m_properties.push_back({ field, getter, sectionIndex });
		}

		m_propertyGroups.push_back(std::move(group));
	}

	RefreshValues();
	FilterProperties();
}

void InspectorPanel::OnInitialize()
{
	auto* layout = MemNew<VBoxContainer>();
	AddNode(layout);
	layout->UpdateSize(GetSize());

	auto* toolsMargin = MemNew<MarginContainer>();
	toolsMargin->AddThemeChangedCallback(
		[control = toolsMargin]()
		{
			control->SetMargins(
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});

	auto* actions = MemNew<HBoxContainer>();
	actions->AddThemeChangedCallback([actions]()
	{
		actions->SetSeparation(static_cast<float>(actions->GetThemeConstant("spacing", "Editor")));
	});
	auto* actionsMargin = MemNew<MarginContainer>();
	actionsMargin->AddThemeChangedCallback([actionsMargin]()
	{
		float const margin = static_cast<float>(actionsMargin->GetThemeConstant("panel_margin", "Editor"));
		actionsMargin->SetMargins(margin, margin, margin, margin);
	});
	actionsMargin->AddNode(actions);
	m_resourceActions = actionsMargin;
	auto* tools = MemNew<HBoxContainer>();
	tools->AddThemeChangedCallback(
		[control = tools]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	toolsMargin->AddNode(tools);

	m_filter = MemNew<LineEdit>();
	m_filter->SetHorizontalSizeFlags(Size_ExpandFill);
	m_filter->SetPlaceholder("Filter properties");
	m_filter->SetIcon(GetThemeIcon("Search", "EditorIcons"));
	m_filter->SetMinimumCharacterWidth(1);
	m_filter->SetTextChangedCallback(
		[this](String const&)
		{
			FilterProperties();
		});
	tools->AddNode(m_filter);
	m_resourceBack = EditorPopupUtils::AddButton(actions, "Back", [this]() { GoBack(); });
	EditorPopupUtils::AddButton(actions, "Save", [this]() { SaveResource(); });
	m_resourceSaveAs = EditorPopupUtils::AddButton(actions, "Save As", [this]() { SaveResourceAs(); });

	m_objectBar = MemNew<PanelContainer>();
	m_objectBar->SetThemeTypeVariation("EditorObjectBar");
	layout->AddNode(m_objectBar);
	layout->AddNode(actionsMargin);
	layout->AddNode(toolsMargin);
	m_previewHost = MemNew<MarginContainer>();
	m_previewHost->AddThemeChangedCallback([host = static_cast<MarginContainer*>(m_previewHost)]()
	{
		float const margin = static_cast<float>(host->GetThemeConstant("panel_margin", "Editor"));
		host->SetMargins(margin, 0.f, margin, margin);
	});
	m_previewHost->SetVisible(false);
	layout->AddNode(m_previewHost);
	auto* objectRow = MemNew<HBoxContainer>();
	objectRow->AddThemeChangedCallback(
		[control = objectRow]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("status_spacing", "Editor")) * 0.5f);
		});
	m_objectBar->AddNode(objectRow);
	m_objectIcon = MemNew<TextureRect>();
	m_objectIcon->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
	m_objectIcon->SetStretchMode(TextureRect::StretchMode::KeepAspectCentered);
	m_objectIcon->AddThemeChangedCallback(
		[control = m_objectIcon]()
		{
			control->SetCustomMinimumSize(Vector2(
				static_cast<float>(control->GetThemeConstant("icon_size", "Button")),
				static_cast<float>(control->GetThemeConstant("icon_size", "Button"))));
		});
	objectRow->AddNode(m_objectIcon);
	m_objectName = EditorPopupUtils::AddLabel(objectRow, "");
	m_objectName->SetHorizontalSizeFlags(Size_ExpandFill);
	m_objectName->SetClipText(true);
	m_objectName->SetVerticalAlignment(VerticalAlignment::Center);
	m_empty      = EditorPopupUtils::AddLabel(layout, "No object selected", "EditorStatusLabel");
	auto* scroll = MemNew<ScrollContainer>();
	scroll->SetFollowFocus(true);
	m_scroll     = scroll;
	scroll->SetVerticalSizeFlags(Size_ExpandFill);
	layout->AddNode(scroll);

	auto* contentMargin = MemNew<MarginContainer>();
	contentMargin->AddThemeChangedCallback(
		[control = contentMargin]()
		{
			control->SetMargins(
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				0.f,
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	scroll->AddNode(contentMargin);

	m_content = MemNew<VBoxContainer>();
	m_content->AddThemeChangedCallback(
		[control = m_content]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	contentMargin->AddNode(m_content);
}

void InspectorPanel::FilterProperties()
{
	if (m_filter == nullptr)
	{
		return;
	}

	auto lower = [](std::string text)
	{
		std::transform(
			text.begin(),
			text.end(),
			text.begin(),
			[](unsigned char c)
			{
				return static_cast<char>(std::tolower(c));
			});
		return text;
	};
	std::string const query = lower(m_filter->GetText().ToUtf8());
	for (PropertyGroup& group : m_propertyGroups)
	{
		bool              any = false;
		std::vector<bool> sectionMatches(group.m_sections.size(), false);
		for (PropertyEntry& entry : group.m_properties)
		{
			PropertySection* section = entry.m_section >= 0 ? &group.m_sections[entry.m_section] : nullptr;
			bool const matches = query.empty()
								 || lower(entry.m_property->GetDisplayName()).find(query) != std::string::npos
								 || (section != nullptr && lower(section->m_name).find(query) != std::string::npos);
			Control* control = entry.m_property->GetControl();
			bool const visible = matches && (section == nullptr || !section->m_collapsed || !query.empty());
			if (control->IsVisibleInHierarchy() && !visible)
			{
				entry.m_property->EndEditing(LineEdit::EditEndReason::FocusLost);
			}

			control->SetVisible(matches);
			if (section != nullptr)
			{
				sectionMatches[entry.m_section] = sectionMatches[entry.m_section] || matches;
			}

			any = any || matches;
		}

		for (size_t i = 0; i < group.m_sections.size(); ++i)
		{
			PropertySection& section = group.m_sections[i];
			section.m_content->SetVisible(sectionMatches[i] && (!section.m_collapsed || !query.empty()));
			section.m_root->SetVisible(sectionMatches[i]);
			section.m_heading->SetIcon(
				GetThemeIcon(section.m_collapsed && query.empty() ? "ChevronRight" : "ChevronDown", "EditorIcons"));
		}

		group.m_header->SetVisible(any);
	}
}

void InspectorPanel::OnRender(EditorUIContext& context)
{
	m_sceneTree          = context.m_sceneTree;
	ObjectID const selected = context.m_selection != nullptr ? context.m_selection->GetSelected() : ObjectID::Invalid;
	if (selected != m_sceneSelection)
	{
		FinishEditing();
		m_resource = {};
		m_resourceHistory.clear();
		m_sceneSelection = selected;
	}

	ObjectID const id = m_resource.IsValid() ? m_resource->GetObjectID() : selected;
	Object* node = GetTarget(id);
	bool const valid = node != nullptr;
	if (!valid || id != m_cachedObjectID)
	{
		ClearProperties();
		m_cachedObjectID = valid ? id : ObjectID::Invalid;
		if (valid)
		{
			BuildInheritanceChain(node->GetClassName());
			RebuildProperties(context);
		}
	}

	m_empty->SetVisible(!valid);
	m_objectBar->SetVisible(valid);
	m_resourceActions->SetVisible(m_resource.IsValid());
	m_resourceBack->SetDisabled(m_resourceHistory.empty());
	m_resourceSaveAs->SetDisabled(dynamic_cast<MaterialResource*>(m_resource.Get()) == nullptr);
	if (valid)
	{
		String label = m_resource.IsValid() ? m_resource->GetName() : static_cast<Node*>(node)->GetName();
		if (m_resource.IsValid() && EditorNode::Get()->GetChecker()->IsResourceDirty(m_resource->GetVirtualPath()))
		{
			label = label + String(" *");
		}

		m_objectName->SetText(label);
		m_objectIcon->SetTexture(GetThemeIcon(node->GetClassName(), "EditorIcons"));
		m_objectIcon->SetTint(EditorIcons::GetIconColor(node->GetClassName(), this));
	}
	for (PropertyGroup& group : m_propertyGroups)
	{
		group.m_icon->SetTexture(GetThemeIcon(group.m_className, "EditorIcons"));
		group.m_icon->SetTint(EditorIcons::GetIconColor(group.m_className, this));
		Label*            title = group.m_title;
		Ref<FontResource> font  = title->GetFont();
		float const       textWidth =
			font.IsValid()
				? g_engine->m_textServer
					  ->GetTextLine(font->GetFontRID(), title->GetText(), static_cast<float>(title->GetFontSize()))
					  .m_width
				: 0.f;
		float const padding = static_cast<float>(GetThemeConstant("content_padding", "Editor"));
		float const inset   = GetThemeConstant("icon_size", "Button") + GetThemeConstant("spacing", "Editor") + padding;
		float const width   = std::max(inset + 1.f, m_scroll->GetPageSize().x - padding);
		float const available = width - inset;
		title->SetCustomMinimumSize(Vector2(std::min(textWidth, available), 0.f));
		group.m_bar->SetPosition(Vector2(m_scroll->GetHorizontalScroll(), 0.f));
		group.m_bar->UpdateSize(Vector2(width, static_cast<float>(GetThemeConstant("section_height", "Editor"))));
	}

	BuildMaterialPreview();
	RefreshValues();
	FilterProperties();
}

void InspectorPanel::FinishInput(EditorUIContext& context)
{
	ObjectID const selected = m_resource.IsValid() ? m_resource->GetObjectID()
		: context.m_selection != nullptr ? context.m_selection->GetSelected() : ObjectID::Invalid;
	Object* node = GetTarget(m_cachedObjectID);
	if (selected != m_cachedObjectID || node == nullptr)
	{
		ClearProperties();
		m_cachedObjectID = ObjectID::Invalid;
	}
}

void InspectorPanel::FinishEditing()
{
	for (PropertyGroup& group : m_propertyGroups)
	{
		for (PropertyEntry& entry : group.m_properties)
		{
			entry.m_property->EndEditing(LineEdit::EditEndReason::FocusLost);
		}
	}

	if (GetViewport() != nullptr)
	{
		GetViewport()->CancelGuiInteraction(this);
	}
}

void InspectorPanel::OnClose()
{
	if (GetViewport() != nullptr)
	{
		GetViewport()->CancelGuiInteraction(this);
	}
}

Control* InspectorPanel::GetContentControl() const
{
	return m_content;
}

void InspectorPanel::BeginPropertyEdit(
	std::string const& name, ObjectID id, MethodBind const* setter, MethodBind const* getter, InspectorProperty* field)
{
	EndPropertyEdit(false);
	Object* target = GetTarget(id);
	Node* scene  = m_sceneTree != nullptr ? m_sceneTree->GetScene() : nullptr;
	if (id != m_cachedObjectID || target == nullptr || (scene == nullptr && !m_resource.IsValid())
		|| setter == nullptr || getter == nullptr)
	{
		field->EndEditing();
		return;
	}

	m_propertyEdit.m_target   = id;
	m_propertyEdit.m_scene    = GetUndoHistory();
	m_propertyEdit.m_name     = name;
	m_propertyEdit.m_setter   = setter;
	m_propertyEdit.m_getter   = getter;
	m_propertyEdit.m_field    = field;
	m_propertyEdit.m_original = getter->Invoke(target, {});
	m_propertyEdit.m_last     = m_propertyEdit.m_original;
}

void InspectorPanel::PreviewPropertyValue(Variant const& value)
{
	Object* target = GetTarget(m_propertyEdit.m_target);
	if (target == nullptr || value.GetType() != m_propertyEdit.m_original.GetType()
		|| m_propertyEdit.m_target != m_cachedObjectID
		|| m_propertyEdit.m_getter->Invoke(target, {}) != m_propertyEdit.m_last)
	{
		EndPropertyEdit(false);
		return;
	}

	m_propertyEdit.m_applying = true;
	m_propertyEdit.m_setter->Invoke(target, { value });
	target = GetTarget(m_propertyEdit.m_target);
	if (target != nullptr)
	{
		m_propertyEdit.m_last = m_propertyEdit.m_getter->Invoke(target, {});
	}

	m_propertyEdit.m_applying = false;
	RefreshValues();
}

void InspectorPanel::EndPropertyEdit(bool accept)
{
	if (!m_propertyEdit.m_target.IsValid())
	{
		return;
	}

	PropertyEdit const edit = m_propertyEdit;
	m_propertyEdit          = {};
	Object* target       = GetTarget(edit.m_target);
	if (target != nullptr)
	{
		Variant const actual = edit.m_getter->Invoke(target, {});
		if (actual == edit.m_last)
		{
			if (accept && GetUndoHistory() == edit.m_scene)
			{
				CommitValue(edit.m_name, edit.m_target, edit.m_setter, edit.m_getter, actual, &edit.m_original);
			}
			else
			{
				edit.m_setter->Invoke(target, { edit.m_original });
			}
		}
	}

	edit.m_field->EndEditing();
}

Object* InspectorPanel::GetTarget(ObjectID id) const
{
	Object* object = ObjectDatabase::GetInstance<Object>(id);
	if (auto* node = dynamic_cast<Node*>(object))
	{
		return !node->IsQueuedForDeletion() && node->GetSceneTree() == m_sceneTree ? node : nullptr;
	}

	return dynamic_cast<Resource*>(object);
}

ObjectID InspectorPanel::GetUndoHistory() const
{
	if (m_resource.IsValid())
	{
		return m_resource->GetObjectID();
	}

	Node* scene = m_sceneTree != nullptr ? m_sceneTree->GetScene() : nullptr;
	return scene != nullptr ? scene->GetObjectID() : ObjectID::Invalid;
}

void InspectorPanel::EditResource(Ref<Resource> const& resource)
{
	if (!resource.IsValid() || resource == m_resource)
	{
		return;
	}

	if (!resource->GetVirtualPath().HasExtension(".tres"))
	{
		VirtualPath const& path = resource->GetSourceFilePath().IsValid() ? resource->GetSourceFilePath() : resource->GetVirtualPath();
		if (path.IsValid())
		{
			auto& files = EditorNode::Get()->m_editorUI->GetPanel<FileSystemPanel>();
			files.NavigateTo(path.GetParent());
			files.SelectPath(path);
		}

		return;
	}

	FinishEditing();
	ClearProperties();
	m_resourceHistory.push_back(m_resource);
	m_resource = resource;
	m_sceneSelection = EditorNode::Get()->GetSelection().GetSelected();
	m_cachedObjectID = ObjectID::Invalid;
}

void InspectorPanel::GoBack()
{
	FinishEditing();
	ClearProperties();
	m_resource = m_resourceHistory.empty() ? Ref<Resource>() : m_resourceHistory.back();
	if (!m_resourceHistory.empty())
	{
		m_resourceHistory.pop_back();
	}

	m_cachedObjectID = ObjectID::Invalid;
}

bool InspectorPanel::SaveResource()
{
	FinishEditing();
	if (!m_resource.IsValid())
	{
		return EditorNode::Get()->SaveScene();
	}

	bool const saved = EditorNode::Get()->GetChecker()->SaveResource(m_resource);
	if (!saved)
	{
		EditorNode::Get()->m_editorUI->Warning("Save Resource Failed", m_resource->GetVirtualPath().GetString());
	}

	return saved;
}

void InspectorPanel::SaveResourceAs()
{
	FinishEditing();
	Ref<MaterialResource> material(m_resource);
	if (material.IsValid())
	{
		EditorNode::Get()->m_editorUI->SaveMaterialAs(material, [this](VirtualPath const& path)
		{
			EditResource(ResourceLoader::Load(path));
		});
	}
	else if (!m_resource.IsValid())
	{
		EditorNode::Get()->RequestSaveSceneAs();
	}
}

void InspectorPanel::BuildMaterialPreview()
{
	Ref<MaterialResource> material(m_resource);
	m_previewHost->SetVisible(material.IsValid());
	if (!material.IsValid())
	{
		if (m_preview != nullptr)
		{
			m_preview->QueueFree();
			m_preview = nullptr;
			m_previewMesh = nullptr;
		}

		return;
	}

	if (m_preview == nullptr)
	{
		m_preview = MemNew<SubViewportContainer>();
		m_preview->SetName("MaterialPreview");
		m_preview->SetCustomMinimumSize(Vector2(0.f, 180.f));
		m_preview->SetTextureFilter(TextureFilter::Linear);
		m_previewHost->AddNode(m_preview);
	}

	if (m_previewMesh == nullptr)
	{
		SubViewport* viewport = m_preview->GetSubViewport();
		auto* camera = MemNew<Camera3D>();
		camera->SetLocalPosition(Vector3(3.f, 0.f, 0.f));
		camera->SetLocalOrientation(EulerAngles(180.f, 0.f, 0.f));
		viewport->AddNode(camera);
		camera->SetCurrent();
		auto* light = MemNew<DirectionalLight3D>();
		light->SetLocalOrientation(EulerAngles(150.f, 35.f, 0.f));
		viewport->AddNode(light);

		Ref<SurfaceTool> surface = CreateRef<SurfaceTool>();
		constexpr int rings = 16;
		constexpr int sectors = 24;
		constexpr float pi = 3.14159265358979323846f;
		auto vertex = [&](int ring, int sector)
		{
			float const v = static_cast<float>(ring) / rings;
			float const u = static_cast<float>(sector) / sectors;
			float const latitude = v * pi;
			float const longitude = u * 2.f * pi;
			Vector3 position(std::sin(latitude) * std::cos(longitude), std::sin(latitude) * std::sin(longitude), std::cos(latitude));
			surface->SetNormal(position);
			surface->SetUV(Vector2(u, v));
			surface->AddVertex(position);
		};
		for (int ring = 0; ring < rings; ++ring)
		{
			for (int sector = 0; sector < sectors; ++sector)
			{
				vertex(ring, sector);
				vertex(ring + 1, sector);
				vertex(ring + 1, sector + 1);
				vertex(ring, sector);
				vertex(ring + 1, sector + 1);
				vertex(ring, sector + 1);
			}
		}

		surface->Index();
		surface->GenerateTangents();
		m_previewMesh = MemNew<MeshInstance3D>();
		m_previewMesh->SetMeshResource(surface->CreateMeshResource());
		viewport->AddNode(m_previewMesh);
	}

	m_previewMesh->SetMaterialOverride(material);
}

void InspectorPanel::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_ExitTree:
		ClearProperties();
		break;
	}
}
