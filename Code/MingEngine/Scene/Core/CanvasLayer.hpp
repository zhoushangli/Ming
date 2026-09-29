#pragma once

#include "MingEngine/Scene/Core/Node.hpp"

class CanvasLayer : public Node
{
	MCLASS(CanvasLayer, Node);

public:
	CanvasLayer();
	~CanvasLayer();

	RID GetCanvasLayerRID() const { return m_canvasLayerRID; }

private:
	void OnNotification(int notification);

	RID m_canvasLayerRID = RID::Invalid;
};
