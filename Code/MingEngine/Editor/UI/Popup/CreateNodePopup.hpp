#pragma once

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/ObjectID.hpp"

#include <map>
#include <string>
#include <vector>

struct EditorUIContext;
class Node;

class CreateNodePopup
{
public:
	void Open(ObjectID parentID);
	void Render(EditorUIContext& context);

private:
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
	char        m_filter[128] = {};
	std::string m_selectedClass;
	ObjectID    m_parentID = ObjectID::Invalid;
	bool        m_openRequested = false;
};
