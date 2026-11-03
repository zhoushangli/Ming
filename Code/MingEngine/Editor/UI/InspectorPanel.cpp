#include "MingEngine/Editor/UI/InspectorPanel.hpp"

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
		if (current == "Node" || info == nullptr || info->m_parentClassName.empty()
			|| info->m_parentClassName == current)
		{
			break;
		}

		current = info->m_parentClassName;
	}
}

void InspectorPanel::RefreshValues()
{
	Node* node = ObjectDatabase::GetInstance<Node>(m_cachedObjectID);
	if (node == nullptr || node->IsQueuedForDeletion() || node->GetSceneTree() != m_sceneTree)
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
	Variant const&     value)
{
	Node* node = ObjectDatabase::GetInstance<Node>(id);
	if (id != m_cachedObjectID || node == nullptr || node->IsQueuedForDeletion() || node->GetSceneTree() != m_sceneTree
		|| setter == nullptr || getter == nullptr)
	{
		return;
	}

	Variant const oldValue = getter->Invoke(node, {});
	if (value.GetType() != oldValue.GetType() || value == oldValue)
	{
		return;
	}

	node = ObjectDatabase::GetInstance<Node>(id);
	if (node == nullptr || node->IsQueuedForDeletion())
	{
		return;
	}

	Ref<Resource> oldResource(oldValue);
	setter->Invoke(node, { value });
	node = ObjectDatabase::GetInstance<Node>(id);
	if (node == nullptr || node->IsQueuedForDeletion())
	{
		return;
	}

	Variant const actual = getter->Invoke(node, {});
	if (actual != oldValue && EditorNode::Get() != nullptr)
	{
		auto* editor  = EditorNode::Get();
		Node* scene   = m_sceneTree->GetScene();
		auto& history = editor->GetUndoRedo();
		if (scene != nullptr && history.CreateAction("Edit " + propertyName, scene->GetObjectID()))
		{
			history.AddDoProperty(node, propertyName, actual);
			history.AddUndoProperty(node, propertyName, oldValue);
			history.AddDoMethod(editor, "MarkSceneDirty");
			history.AddUndoMethod(editor, "MarkSceneDirty");
			history.CommitAction(false);
		}
		editor->MarkSceneDirty();
	}

	RefreshValues();
}

void InspectorPanel::RebuildProperties(EditorUIContext& context)
{
	Node* node = ObjectDatabase::GetInstance<Node>(m_cachedObjectID);
	if (node == nullptr || node->IsQueuedForDeletion() || node->GetSceneTree() != context.m_sceneTree)
	{
		return;
	}

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
		int         sectionIndex = -1;
		for (PropertyInfo const& prop : ClassDatabase::GetProperties(className))
		{
			if (prop.HasUsage(PropertyInfo::UsageFlags::Group))
			{
				sectionName  = prop.m_name;
				sectionIndex = -1;
				continue;
			}

			if (!prop.HasUsage(PropertyInfo::UsageFlags::Inspector))
			{
				continue;
			}

			std::string const desiredSection =
				className == "Node3D"
						&& (prop.m_name == "position" || prop.m_name == "rotation" || prop.m_name == "scale")
					? "Transform"
					: sectionName;
			if (!desiredSection.empty()
				&& (sectionIndex < 0 || group.m_sections[sectionIndex].m_name != desiredSection))
			{
				Button* sectionHeading = EditorPopupUtils::AddButton(
					m_content,
					desiredSection,
					[this, className, desiredSection]()
					{
						for (PropertyGroup& current : m_propertyGroups)
						{
							if (current.m_className != className)
							{
								continue;
							}

							for (PropertySection& section : current.m_sections)
							{
								if (section.m_name == desiredSection)
								{
									section.m_collapsed = !section.m_collapsed;
								}
							}
						}

						FilterProperties();
					});
				sectionHeading->SetThemeTypeVariation("EditorGroupButton");
				sectionHeading->SetHorizontalAlignment(HorizontalAlignment::Left);
				group.m_sections.push_back({ desiredSection, sectionHeading, false });
				sectionIndex = static_cast<int>(group.m_sections.size()) - 1;
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

			field->SetValueReader(
				[id, getter]() -> Variant
				{
					Node* target = ObjectDatabase::GetInstance<Node>(id);
					if (target == nullptr || target->IsQueuedForDeletion() || getter == nullptr)
					{
						return {};
					}

					return getter->Invoke(target, {});
				});
			m_content->AddNode(field->GetControl());
			group.m_properties.push_back({ field, getter, desiredSection.empty() ? -1 : sectionIndex });
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

	m_objectBar = MemNew<PanelContainer>();
	m_objectBar->SetThemeTypeVariation("EditorObjectBar");
	layout->AddNode(m_objectBar);
	layout->AddNode(toolsMargin);
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
			entry.m_property->GetControl()->SetVisible(
				matches && (section == nullptr || !section->m_collapsed || !query.empty()));
			if (section != nullptr)
			{
				sectionMatches[entry.m_section] = sectionMatches[entry.m_section] || matches;
			}

			any = any || matches;
		}

		for (size_t i = 0; i < group.m_sections.size(); ++i)
		{
			PropertySection& section = group.m_sections[i];
			section.m_heading->SetVisible(sectionMatches[i]);
			section.m_heading->SetIcon(
				GetThemeIcon(section.m_collapsed && query.empty() ? "ChevronRight" : "ChevronDown", "EditorIcons"));
		}

		group.m_header->SetVisible(any);
	}
}

void InspectorPanel::OnRender(EditorUIContext& context)
{
	m_sceneTree          = context.m_sceneTree;
	ObjectID const id    = context.m_selection != nullptr ? context.m_selection->GetSelected() : ObjectID::Invalid;
	Node*          node  = ObjectDatabase::GetInstance<Node>(id);
	bool const     valid = node != nullptr && !node->IsQueuedForDeletion() && node->GetSceneTree() == m_sceneTree;
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
	if (valid)
	{
		m_objectName->SetText(node->GetName());
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

	RefreshValues();
	FilterProperties();
}

void InspectorPanel::FinishInput(EditorUIContext& context)
{
	ObjectID const selected = context.m_selection != nullptr ? context.m_selection->GetSelected() : ObjectID::Invalid;
	Node*          node     = ObjectDatabase::GetInstance<Node>(m_cachedObjectID);
	if (selected != m_cachedObjectID || node == nullptr || node->IsQueuedForDeletion())
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
