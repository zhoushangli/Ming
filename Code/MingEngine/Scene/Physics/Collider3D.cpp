#include "MingEngine/Scene/Physics/Collider3D.hpp"

size_t Collider3D::s_callbackUID = 0;

size_t Collider3D::AddCollideCallback(CollideCallbackFunction const& callback)
{
	size_t callbackId = s_callbackUID;
	s_callbackUID++;
	m_collideCallbacks[callbackId] = callback;
	return callbackId;
}

bool Collider3D::RemoveCollideCallback(size_t callbackId)
{
	auto iter = m_collideCallbacks.find(callbackId);
	if (iter != m_collideCallbacks.end())
	{
		m_collideCallbacks.erase(iter);
		return true;
	}
	return false;
}

void Collider3D::NotifyCollide(CollideInfo const& info) const
{
	for (const auto& pair : m_collideCallbacks)
	{
		pair.second(info);
	}
}
