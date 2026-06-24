#include "MingEngine/Core/Object/RefCounted.hpp"

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
	return true;
}

bool RefCounted::RemoveRef()
{
	if (m_refCount <= 0)
	{
		return false;
	}

	m_refCount--;
	return (m_refCount == 0);
}

int RefCounted::GetRefCount() const { return m_refCount; }

void RefCounted::BindMethods() {}