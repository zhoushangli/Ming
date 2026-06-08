#pragma once

#include "MingEngine/Scene/Core/ClassDatabase.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include <map>
#include <string>
#include <vector>

struct EditorUIContext;
class Node;

class CreateNodePanel
{
public:
	void Open(NodeHandle parentHandle);
	void Render(EditorUIContext& context);
	bool IsOpen() const;

private:
	bool RenderClassNode(ClassDatabase::ClassInfo const* classInfo,
		std::map<std::string, std::vector<ClassDatabase::ClassInfo const*>> const& childrenByClass,
		std::string const& filterText,
		EditorUIContext& context);
	bool DoesClassBranchMatch(ClassDatabase::ClassInfo const* classInfo,
		std::map<std::string, std::vector<ClassDatabase::ClassInfo const*>> const& childrenByClass,
		std::string const& filterText) const;
	bool        CreateSelectedNode(EditorUIContext& context);
	Node*       ResolveCreateParent(EditorUIContext const& context) const;
	std::string MakeUniqueNodeName(Node const* parent, std::string const& className) const;
	void        Reset();

private:
	char        m_filter[128] = {};
	std::string m_selectedClass;
	NodeHandle  m_parentHandle = NodeHandle::Invalid;
	bool        m_openPopup    = false;
	bool        m_isOpen       = false;
};
