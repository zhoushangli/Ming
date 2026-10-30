#pragma once

#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Core/String.hpp"

struct GuiDragData
{
	String   m_type;
	ObjectID m_objectID = ObjectID::Invalid;
	String   m_value;
};

enum class HorizontalAlignment
{
	Left,
	Center,
	Right,
	Fill
};

enum class VerticalAlignment
{
	Top,
	Center,
	Bottom,
	Fill
};
