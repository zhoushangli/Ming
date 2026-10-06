#pragma once

#include "MingEngine/Scene/GUI/BoxContainer.hpp"

class VBoxContainer : public BoxContainer
{
	MCLASS(VBoxContainer, BoxContainer);

public:
	VBoxContainer() : BoxContainer(true) {}

protected:
	static void BindMethods() {}
};
