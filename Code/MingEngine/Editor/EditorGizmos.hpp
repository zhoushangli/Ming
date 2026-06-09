#pragma once

#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

class EditorGizmos : public Node
{
	MCLASS(EditorGizmos, Node);

public:
	EditorGizmos();
	~EditorGizmos() override;

	void SetWorldGridVisible(bool visible);
	void SetWorldAxisVisible(bool visible);

protected:
	void OnReady() override;

private:
	NodeHandle m_worldGridHandle = NodeHandle::Invalid;
	NodeHandle m_worldAxisHandle = NodeHandle::Invalid;
};
