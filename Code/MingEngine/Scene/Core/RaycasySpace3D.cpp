#include "MingEngine/Scene/Core/RaycasySpace3D.hpp"

RaycastSpace3D::~RaycastSpace3D()
{
	for (RaycastObject* object : m_raycastObjects)
	{
		if (object != nullptr)
		{
			delete object;
		}
	}
}