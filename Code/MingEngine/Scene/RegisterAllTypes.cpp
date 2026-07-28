#include "MingEngine/Scene/RegisterAllTypes.hpp"

#include "MingEngine/Core/Object/RegisterCoreTypes.hpp"
#include "MingEngine/Engine/Application/RegisterEngineTypes.hpp"
#include "MingEngine/Scene/RegisterSceneTypes.hpp"

void RegisterAllTypes()
{
	RegisterCoreTypes();
	RegisterEngineTypes();
	RegisterSceneTypes();
}
