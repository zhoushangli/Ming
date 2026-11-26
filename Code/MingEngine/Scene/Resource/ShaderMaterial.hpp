#pragma once

#include "MingEngine/Scene/Resource/MaterialResource.hpp"

class ShaderMaterial : public MaterialResource
{
	MCLASS(ShaderMaterial, MaterialResource)

public:
	ShaderMaterial() = default;

protected:
	static void BindMethods() {}
};
