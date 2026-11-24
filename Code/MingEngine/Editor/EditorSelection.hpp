#pragma once

#include "MingEngine/Core/Object/ObjectID.hpp"

class EditorSelection
{
public:
	ObjectID GetSelected() const;
	void SetSelected(ObjectID nodeID);
	void Clear();

private:
	ObjectID m_selectedNodeID = ObjectID::Invalid;
};
