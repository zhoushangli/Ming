#include "MingEngine/Scene/RegisterAllTypes.hpp"

#include "MingEngine/Core/Object/RegisterCoreTypes.hpp"
#include "MingEngine/Engine/Application/RegisterEngineTypes.hpp"
#include "MingEngine/Scene/RegisterSceneTypes.hpp"

#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

void RegisterAllTypes()
{
	RegisterCoreTypes();
	RegisterEngineTypes();
	RegisterSceneTypes();
	ThemeDatabase::Startup();
}
