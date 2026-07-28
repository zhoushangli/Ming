#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Scene/3D/VisualInstance3D.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

class Skybox3D : public VisualInstance3D
{
public:
	explicit Skybox3D(VirtualPath const &imagePath);
	~Skybox3D() override;

protected:
	RenderRequest SubmitRenderRequest() const override;

protected:
	VirtualPath m_imagePath;
	Ref<TextureResource> m_textureRef;
	VertexBuffer *m_vertexBuffer = nullptr;
	IndexBuffer *m_indexBuffer = nullptr;
};
