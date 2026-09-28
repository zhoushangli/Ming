#include "MingEngine/Scene/Resource/MaterialResource.hpp"

#include <utility>

bool MaterialResource::CopyFrom(Resource&& other)
{
	// 1) Validate type
	MaterialResource* otherMaterial = dynamic_cast<MaterialResource*>(&other);
	if (otherMaterial == nullptr)
	{
		return false;
	}

	// 2) Move CPU data fields; a moved Ref leaves the source empty
	MoveBaseFrom(std::move(other));
	m_shaderResource   = std::move(otherMaterial->m_shaderResource);
	m_textureResources = std::move(otherMaterial->m_textureResources);
	m_tint             = otherMaterial->m_tint;
	m_blendMode        = otherMaterial->m_blendMode;
	m_depthMode        = otherMaterial->m_depthMode;
	m_rasterizerMode   = otherMaterial->m_rasterizerMode;
	m_samplerMode      = otherMaterial->m_samplerMode;

	return true;
}