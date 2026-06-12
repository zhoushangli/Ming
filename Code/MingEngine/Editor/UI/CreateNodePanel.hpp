#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Scene/Core/ClassDatabase.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include <map>
#include <string>
#include <vector>

struct EditorUIContext;
class Node;

struct CreateNodePanelData final : public UIData
{
	NodeHandle m_parentHandle = NodeHandle::Invalid;
};

class CreateNodePanel final : public EditorPanel
{
public:
	using Data = CreateNodePanelData;

	CreateNodePanel();

private:
	void OnOpen(UIData const& data) override;
	void OnClose() override;
	void OnRender(EditorUIContext& context) override;
	bool RenderClassNode(
		ClassInfo const* classInfo,
		std::map<std::string, std::vector<ClassInfo const*>> const& childrenByClass,
		std::string const& filterText,
		EditorUIContext& context);
	bool DoesClassBranchMatch(
		ClassInfo const* classInfo,
		std::map<std::string, std::vector<ClassInfo const*>> const& childrenByClass,
		std::string const& filterText) const;
	bool CreateSelectedNode(EditorUIContext& context);
	Node* ResolveCreateParent(EditorUIContext const& context) const;
	void Reset();

private:
	char m_filter[128] = {};
	std::string m_selectedClass;
	NodeHandle m_parentHandle = NodeHandle::Invalid;
	bool m_openPopup          = false;
	bool m_openWarningPopup   = false;
};
