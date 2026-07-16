#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"

AABBCollider3D::AABBCollider3D(AABB3 const& localBounds) : m_localBounds(localBounds) {}

void AABBCollider3D::RenderDebug() const {}
