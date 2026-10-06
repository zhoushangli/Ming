#pragma once

#include "MingEngine/Scene/GUI/BoxContainer.hpp"

class HBoxContainer : public BoxContainer
{
	MCLASS(HBoxContainer, BoxContainer);

public:
	HBoxContainer() : BoxContainer(false) {}

protected:
	static void BindMethods() {}
};
