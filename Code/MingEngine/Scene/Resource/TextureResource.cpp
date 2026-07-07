#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/GPUTexture.hpp"

TextureResource::~TextureResource()
{
	delete m_gpuTexture;
	m_gpuTexture = nullptr;
}

bool TextureResource::IsEmpty() const { return m_pixels.empty() || m_dimensions.x == 0 || m_dimensions.y == 0; }

bool TextureResource::CopyFrom(Resource const& other)
{
	// 1) Validate type
	TextureResource const* otherTex = dynamic_cast<TextureResource const*>(&other);
	if (otherTex == nullptr)
	{
		return false;
	}

	// 2) Copy CPU data fields
	m_format     = otherTex->m_format;
	m_channels   = otherTex->m_channels;
	m_dimensions = otherTex->m_dimensions;
	m_pixels     = otherTex->m_pixels;

	// 3) Recreate GPU texture from copied CPU data
	InitGPUResources();

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
