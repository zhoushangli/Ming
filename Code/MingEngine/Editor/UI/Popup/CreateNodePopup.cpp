#include "MingEngine/Editor/UI/Popup/CreateNodePopup.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

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

bool CanCreateClass(ClassInfo const* classInfo)
{
	return classInfo != nullptr && !classInfo->m_isVirtual && classInfo->m_creator;
}
} // namespace

void CreateNodePopup::Open(ObjectID parentID)
{
	Reset();
	m_parentID      = parentID;
	m_openRequested = true;
}

void CreateNodePopup::Render(EditorUIContext& context)
{
	constexpr char const* popupId = "Create New Node";
	if (m_openRequested)
	{
		ImGui::OpenPopup(popupId);
		m_openRequested = false;
	}

	if (!EditorPopupUtils::BeginModal(popupId, ImVec2(620.f, 540.f)))
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
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	if (ImGui::BeginChild("CreateNodeTypeTree", ImVec2(0.f, -footerSize.y), true))
	{
		if (nodeClass != nullptr)
		{
			RenderClassNode(nodeClass, childrenByClass, m_filter, context);
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();

	ClassInfo const* selectedInfo = ClassDatabase::GetClassInfo(m_selectedClass);
	bool const       canCreate    = CanCreateClass(selectedInfo);
	EditorPopupUtils::BeginButtonRow(2);
	ImGui::BeginDisabled(!canCreate);
	bool const createPressed = EditorPopupUtils::ConfirmButton();
	ImGui::EndDisabled();
	ImGui::SameLine(0.f, 64.f);
	bool const cancelPressed = EditorPopupUtils::CancelButton();
	if (cancelPressed)
	{
		ImGui::CloseCurrentPopup();
		Reset();
	}
	else if (createPressed && CreateSelectedNode(context))
	{
		ImGui::CloseCurrentPopup();
		Reset();
	}

	EditorPopupUtils::EndModal();
}

bool CreateNodePopup::RenderClassNode(
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

	bool const canCreate = CanCreateClass(classInfo);
	ImGui::PushID(classInfo->m_className.c_str());
	bool const   isOpen             = ImGui::TreeNodeEx("##CreateNodeClass", flags);
	ImVec2 const rowMin             = ImGui::GetItemRectMin();
	ImVec2 const rowMax             = ImGui::GetItemRectMax();
	bool const   rowHovered         = ImGui::IsMouseHoveringRect(rowMin, rowMax);
	bool const   classClicked       = rowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
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
				Reset();
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

bool CreateNodePopup::DoesClassBranchMatch(
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

bool CreateNodePopup::CreateSelectedNode(EditorUIContext& context)
{
	if (m_selectedClass.empty())
	{
		return false;
	}

	ClassInfo const* selectedInfo = ClassDatabase::GetClassInfo(m_selectedClass);
	if (!CanCreateClass(selectedInfo))
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
	// If the scene is empty, we will make the new node the root of the scene.
	if (sceneRoot == nullptr)
	{
		context.m_sceneTree->ChangeScene(node);
	}
	else
	{
		Node* parent = ResolveCreateParent(context);
		// If the parent is invalid, we will add the new node to the scene root.
		if (sceneRoot != nullptr && parent == nullptr)
		{
			parent = sceneRoot;
		}
		node->SetName(m_selectedClass);

		parent->AddNode(node);
	}

	EditorNode::Get()->MarkSceneDirty();

	if (context.m_selection != nullptr)
	{
		context.m_selection->SetSelected(node->GetObjectID());
	}
	return true;
}

Node* CreateNodePopup::ResolveCreateParent(EditorUIContext const& context) const
{
	if (context.m_sceneTree == nullptr || !m_parentID.IsValid())
	{
		return nullptr;
	}

	// Resolve at creation time because the context node may disappear while the modal is open.
	Node* parent = ObjectDatabase::GetInstance<Node>(m_parentID);
	if (parent == nullptr || parent->GetSceneTree() != context.m_sceneTree)
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

void CreateNodePopup::Reset()
{
	m_selectedClass.clear();
	m_filter[0]     = '\0';
	m_parentID      = ObjectID::Invalid;
	m_openRequested = false;
}
