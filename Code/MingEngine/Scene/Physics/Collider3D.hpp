#pragma once

#include "MingEngine/Scene/3D/Node3D.hpp"

#include <functional>
#include <map>

class Collider3D;

struct CollideInfo
{
	CollideInfo(Collider3D const* collider) : m_collider(collider) {}

	Collider3D const* m_collider = nullptr;
};

class Collider3D : public Node3D
{
	MCLASS(Collider3D, Node3D);

	using CollideCallbackFunction = std::function<void(CollideInfo const&)>;

public:
	Collider3D()           = default;
	~Collider3D() override = default;

	size_t AddCollideCallback(CollideCallbackFunction const& callback);
	bool   RemoveCollideCallback(size_t callbackId);
	void   NotifyCollide(CollideInfo const& info) const;

public:
	Node3D* m_owner     = nullptr;
	bool    m_isEnabled = true;

protected:
	std::map<size_t, CollideCallbackFunction> m_collideCallbacks;
	static size_t                             s_callbackUID;
};
