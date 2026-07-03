#include "MingEngine/Editor/UI/CreateNodePanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <cctype>

namespace
{
std::string ToLower(std::string const& text)
{
	std::string lowerText = text;
	std::transform(
		lowerText.begin(),
		lowerText.end(),
		lowerText.begin(),
		[](unsigned char character) { return static_cast<char>(std::tolower(character)); });
	return lowerText;
}

bool ContainsCaseInsensitive(std::string const& text, std::string const& filterText)
{
	return filterText.empty() || ToLower(text).find(ToLower(filterText)) != std::string::npos;
}

bool CanCreateClassInPanel(ClassInfo const* classInfo)
{
	return classInfo != nullptr && classInfo->m_canCreateInEditor && classInfo->m_creator;
}
} // namespace

CreateNodePanel::CreateNodePanel() : EditorPanel("Create New Node", false) {}

void CreateNodePanel::OnOpen(UIData const& data)
{
	CreateNodePanelData const* createData = dynamic_cast<CreateNodePanelData const*>(&data);
	ASSERT_RECOVERABLE(createData != nullptr, "CreateNodePanel opened with invalid UIData.");
	if (createData == nullptr)
	{
		Close();
		return;
	}

	Reset();
	m_parentHandle = createData->m_parentHandle;
	m_openPopup    = true;
}

void CreateNodePanel::OnClose() { Reset(); }

void CreateNodePanel::OnRender(EditorUIContext& context)
{
	if (m_openPopup)
	{
		ImGui::OpenPopup(GetTitle());
		m_openPopup = false;
	}

	ImGui::SetNextWindowSize(ImVec2(620.f, 540.f), ImGuiCond_FirstUseEver);
	if (!ImGui::BeginPopupModal(GetTitle(), GetOpenState(), ImGuiWindowFlags_NoCollapse))
	{
		return;
	}

	ImGui::TextUnformatted("Search:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.f);
	ImGui::InputTextWithHint("##CreateNodeSearch", "Search node types", m_filter, sizeof(m_filter));
	ImGui::Separator();

	std::vector<ClassInfo const*> classes = ClassDatabase::GetRegisteredClasses(true);

	std::map<std::string, std::vector<ClassInfo const*>> childrenByClass;
	ClassInfo const*                                     nodeClass = nullptr;
	for (ClassInfo const* classInfo : classes)
	{
		if (classInfo == nullptr)
		{
			continue;
		}
		if (classInfo->m_className == Node::GetStaticClassName())
		{
			nodeClass = classInfo;
			continue;
		}
		if (!ClassDatabase::IsSubclassOf(classInfo->m_className, Node::GetStaticClassName()))
		{
			continue;
		}

		childrenByClass[classInfo->m_parentClassName].push_back(classInfo);
	}

	ImVec2 const footerSize(0.f, ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y);
	if (ImGui::BeginChild("CreateNodeTypeTree", ImVec2(0.f, -footerSize.y), true))
	{
		if (nodeClass != nullptr)
		{
			RenderClassNode(nodeClass, childrenByClass, m_filter, context);
		}
	}
	ImGui::EndChild();

	ClassInfo const* selectedInfo = ClassDatabase::GetClassInfo(m_selectedClass);
	bool const       canCreate    = CanCreateClassInPanel(selectedInfo);
	ImGui::BeginDisabled(!canCreate);
	bool const createPressed = ImGui::Button("Create", ImVec2(120.f, 0.f));
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(120.f, 0.f)))
	{
		ImGui::CloseCurrentPopup();
		Close();
	}
	else if (createPressed && CreateSelectedNode(context))
	{
		ImGui::CloseCurrentPopup();
		Close();
	}

	ImGui::EndPopup();
}

bool CreateNodePanel::RenderClassNode(
	ClassInfo const*                                            classInfo,
	std::map<std::string, std::vector<ClassInfo const*>> const& childrenByClass,
	std::string const&                                          filterText,
	EditorUIContext&                                            context)
{
	if (classInfo == nullptr || !DoesClassBranchMatch(classInfo, childrenByClass, filterText))
	{
		return false;
	}

	auto const childrenIter       = childrenByClass.find(classInfo->m_className);
	bool const hasVisibleChildren = childrenIter != childrenByClass.end()
									&& std::any_of(
										childrenIter->second.begin(),
										childrenIter->second.end(),
										[this, &childrenByClass, &filterText](ClassInfo const* child)
										{ return DoesClassBranchMatch(child, childrenByClass, filterText); });

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (!hasVisibleChildren)
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (!filterText.empty())
	{
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	if (m_selectedClass == classInfo->m_className)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool const canCreate = CanCreateClassInPanel(classInfo);
	ImGui::PushID(classInfo->m_className.c_str());
	bool const   isOpen = ImGui::TreeNodeEx("##CreateNodeClass", flags);
	ImVec2 const rowMin = ImGui::GetItemRectMin();
	ImVec2 const rowMax = ImGui::GetItemRectMax();
	bool const   rowHovered = ImGui::IsMouseHoveringRect(rowMin, rowMax);
	bool const   classClicked = rowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	bool const   classDoubleClicked = rowHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

	if (!canCreate)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
	}
	EditorUIWidgets::RenderTreeRowContent(
		classInfo->m_className,
		Node::GetStaticClassName(),
		classInfo->m_className,
		rowMin,
		rowMax,
		EditorUIStyle::SceneTreeIconSize());
	if (!canCreate)
	{
		ImGui::PopStyleColor();
	}

	if (classClicked)
	{
		if (canCreate)
		{
			m_selectedClass = classInfo->m_className;
			if (classDoubleClicked && CreateSelectedNode(context))
			{
				ImGui::CloseCurrentPopup();
				Close();
				if (isOpen && hasVisibleChildren)
				{
					ImGui::TreePop();
				}
				ImGui::PopID();
				return true;
			}
		}
		else
		{
			m_selectedClass.clear();
		}
	}

	if (isOpen && hasVisibleChildren)
	{
		for (ClassInfo const* child : childrenIter->second)
		{
			if (RenderClassNode(child, childrenByClass, filterText, context))
			{
				ImGui::TreePop();
				ImGui::PopID();
				return true;
			}
		}
		ImGui::TreePop();
	}

	ImGui::PopID();
	return false;
}

bool CreateNodePanel::DoesClassBranchMatch(
	ClassInfo const*                                            classInfo,
	std::map<std::string, std::vector<ClassInfo const*>> const& childrenByClass,
	std::string const&                                          filterText) const
{
	if (classInfo == nullptr)
	{
		return false;
	}
	if (ContainsCaseInsensitive(classInfo->m_className, filterText))
	{
		return true;
	}

	auto const childrenIter = childrenByClass.find(classInfo->m_className);
	if (childrenIter == childrenByClass.end())
	{
		return false;
	}
	for (ClassInfo const* child : childrenIter->second)
	{
		if (DoesClassBranchMatch(child, childrenByClass, filterText))
		{
			return true;
		}
	}
	return false;
}

bool CreateNodePanel::CreateSelectedNode(EditorUIContext& context)
{
	if (m_selectedClass.empty())
	{
		return false;
	}

	ClassInfo const* selectedInfo = ClassDatabase::GetClassInfo(m_selectedClass);
	if (!CanCreateClassInPanel(selectedInfo))
	{
		return false;
	}

	Object* object = ClassDatabase::CreateInstance(m_selectedClass);
	Node*   node   = dynamic_cast<Node*>(object);
	if (node == nullptr)
	{
		delete object;
		return false;
	}
	if (context.m_sceneTree == nullptr)
	{
		delete node;
		return false;
	}

	Node* sceneRoot = context.m_sceneTree->GetScene();
	Node* parent    = ResolveCreateParent(context);
	if (sceneRoot != nullptr && parent == nullptr)
	{
		parent = sceneRoot;
	}
	node->SetName(m_selectedClass);

	if (sceneRoot == nullptr)
	{
		// SceneTree's internal Viewport is not the user scene root.
		context.m_sceneTree->ChangeScene(node);
	}
	else
	{
		parent->AddNode(node);
	}

	if (context.m_selection != nullptr)
	{
		context.m_selection->SetSelected(node->GetHandle());
	}
	return true;
}

Node* CreateNodePanel::ResolveCreateParent(EditorUIContext const& context) const
{
	if (context.m_sceneTree == nullptr || !m_parentHandle.IsValid())
	{
		return nullptr;
	}

	// Resolve at creation time because the context node may disappear while the modal is open.
	Node* parent = context.m_sceneTree->ResolveNode(m_parentHandle);
	if (parent == nullptr)
	{
		return nullptr;
	}

	if (!parent->GetSerializable())
	{
		EditorNode::Get()->m_editorUI->Warning("Cannot Create Node", "Cannot add a child to a non-serializable node.");
		return nullptr;
	}

	return parent;
}

void CreateNodePanel::Reset()
{
	m_selectedClass.clear();
	m_filter[0]    = '\0';
	m_parentHandle = NodeHandle::Invalid;
	m_openPopup    = false;
}
