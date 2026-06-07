#pragma once

#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

class EditorGizmos : public Node
{
	CLASS(EditorGizmos, Node);

public:
	EditorGizmos()           = default;
	~EditorGizmos() override = default;

	void SetWorldGridVisible(bool visible);
	void SetWorldAxisVisible(bool visible);

protected:
	void OnReady() override;

private:
	NodeHandle m_worldGridHandle = NodeHandle::Invalid;
	NodeHandle m_worldAxisHandle = NodeHandle::Invalid;
};
