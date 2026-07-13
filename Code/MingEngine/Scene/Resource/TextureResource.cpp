#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/GPUTexture.hpp"

#include <utility>

TextureResource::~TextureResource()
{
	delete m_gpuTexture;
	m_gpuTexture = nullptr;
}

bool TextureResource::IsEmpty() const { return m_pixels.empty() || m_dimensions.x == 0 || m_dimensions.y == 0; }

bool TextureResource::MoveFrom(Resource&& other)
{
	// 1) Validate type
	TextureResource* otherTex = dynamic_cast<TextureResource*>(&other);
	if (otherTex == nullptr)
	{
		return false;
	}

	// 2) Move CPU data fields
	MoveBaseFrom(std::move(other));
	m_format     = std::move(otherTex->m_format);
	m_channels   = otherTex->m_channels;
	m_dimensions = otherTex->m_dimensions;
	m_pixels     = std::move(otherTex->m_pixels);

	// 3) Take ownership of the loaded GPU texture
	delete m_gpuTexture;
	m_gpuTexture = nullptr;
	std::swap(m_gpuTexture, otherTex->m_gpuTexture);

	return true;
}

void TextureResource::InitGPUResources()
{
	// 1) Destroy old GPU texture
	delete m_gpuTexture;
	m_gpuTexture = nullptr;

	// 2) Create new GPU texture from CPU pixel data
	if (g_engine != nullptr && g_engine->m_renderer != nullptr && !IsEmpty())
	{
		m_gpuTexture =
			g_engine->m_renderer->CreateGPUTexture(GetName().c_str(), m_dimensions, m_channels, m_pixels.data());
	}
}
