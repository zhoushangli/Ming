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

bool RefCounted::Ref()
{
	if (m_refCount <= 0)
	{
		return false;
	}

	m_refCount++;
	return true;
}

bool RefCounted::Unref()
{
	if (m_refCount <= 0)
	{
		return false;
	}

	m_refCount--;
	if (m_refCount == 0)
	{
		delete this;
		return true;
	}

	return false;
}

int RefCounted::GetRefCount() const { return m_refCount; }

void RefCounted::BindMethods() {}