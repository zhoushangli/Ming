#include "MingEngine/Editor/EditorSelection.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"
#include "MingEngine/Core/StringUtils.hpp"

namespace
{
std::string FormatObjectID(ObjectID objectID)
{
	return Stringf("uid=%u index=%u", objectID.GetUID(), objectID.GetIndex());
}

}

ObjectID EditorSelection::GetSelected() const
{
	return m_selectedNodeID;
}

void EditorSelection::SetSelected(ObjectID nodeID)
{
	if (m_selectedNodeID == nodeID)
	{
		return;
	}

	ObjectID const previousNodeID = m_selectedNodeID;
	m_selectedNodeID              = nodeID;

	if (!previousNodeID.IsValid() && m_selectedNodeID.IsValid())
	{
		DebugGizmos::AddMessage(
			Stringf("Selected Node: %s", FormatObjectID(m_selectedNodeID).c_str()),
			5.f,
			Color::White,
			Color::White);
		return;
	}

	if (previousNodeID.IsValid() && m_selectedNodeID.IsValid())
	{
		DebugGizmos::AddMessage(
			Stringf(
				"Selection Changed: %s -> %s",
				FormatObjectID(previousNodeID).c_str(),
				FormatObjectID(m_selectedNodeID).c_str()),
			5.f,
			Color::White,
			Color::White);
		return;
	}

	if (previousNodeID.IsValid() && !m_selectedNodeID.IsValid())
	{
		DebugGizmos::AddMessage("Selection Cleared", 5.f, Color::White, Color::White);
	}
}

void EditorSelection::Clear()
{
	SetSelected(ObjectID::Invalid);
}

