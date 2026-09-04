#include "MingEngine/Core/Object/ManagedGCHandle.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include <utility>

ManagedGCHandle::ManagedGCHandle(ManagedGCHandle&& other) : m_value(std::exchange(other.m_value, nullptr)) {}

ManagedGCHandle& ManagedGCHandle::operator=(ManagedGCHandle&& other)
{
	ASSERT_OR_DIE(m_value == nullptr, "ManagedGCHandle must be empty before move assignment.");

	m_value = std::exchange(other.m_value, nullptr);
	return *this;
}

ManagedGCHandle::~ManagedGCHandle()
{
	ASSERT_OR_DIE(m_value == nullptr, "ManagedGCHandle must be empty before destruction.");
}

void ManagedGCHandle::Release()
{
	void* value = std::exchange(m_value, nullptr);

	if (value == nullptr)
	{
		return;
	}

	GUARANTEE_OR_DIE(
		g_engine != nullptr && g_engine->m_scriptSystem != nullptr,
		"Cannot release a GCHandle without an active ScriptSystem.");

	g_engine->m_scriptSystem->ReleaseGCHandle(value);
}
