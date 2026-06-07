#pragma once

#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"

class Skybox3D : public VisualizeInstance3D
{
public:
	explicit Skybox3D(std::string const& imagePath);
	~Skybox3D() override;

protected:
	RenderRequest SubmitRenderRequest() const override;

protected:
	std::string   m_imagePath;
	Texture*      m_texture      = nullptr;
	VertexBuffer* m_vertexBuffer = nullptr;
	IndexBuffer*  m_indexBuffer  = nullptr;
};
