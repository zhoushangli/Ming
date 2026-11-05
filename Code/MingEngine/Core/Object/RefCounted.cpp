#include "MingEngine/Core/Object/RefCounted.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"

bool RefCounted::InitRef()
{
	if (m_refCount != 0)
	{
		return false;
	}

	m_refCount = 1;
	return true;
}

bool RefCounted::AddRef()
{
	if (m_refCount <= 0)
	{
		return false;
	}

	m_refCount++;

	if (m_refCount == 2)
	{
		UpdateNativeBindingGCHandle();
	}

	return true;
}

bool RefCounted::RemoveRef()
{
	if (m_refCount <= 0)
	{
		return false;
	}

	m_refCount--;

	if (m_refCount == 1)
	{
		UpdateNativeBindingGCHandle();
	}

	return (m_refCount == 0);
}

int RefCounted::GetRefCount() const
{
	return m_refCount;
}

void RefCounted::UpdateNativeBindingGCHandle()
{
	if (!m_nativeBindingGCHandle.IsValid() || g_engine == nullptr || g_engine->m_scriptSystem == nullptr)
	{
		return;
	}

	void* previous = m_nativeBindingGCHandle.GetValue();
	void* updated = g_engine->m_scriptSystem->UpdateNativeBindingGCHandle(previous, m_refCount);

	if (updated != previous)
	{
		m_nativeBindingGCHandle.Detach();
		m_nativeBindingGCHandle = ManagedGCHandle(updated);
	}
}

void RefCounted::BindMethods()
{
}
