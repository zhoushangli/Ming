#include "MingEngine/Scene/RegisterAllTypes.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"

#pragma region Engine

void RegisterEngineTypes()
{
	// Engine system types
	ClassDatabase::RegisterClass<SystemBase>(false, false);
	ClassDatabase::RegisterClass<InputSystem>(false, false);

	// Global objects
	ClassDatabase::RegisterGlobalObject(g_engine->m_inputSystem);
}

#pragma endregion
