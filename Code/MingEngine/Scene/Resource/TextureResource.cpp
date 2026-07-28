#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/GPUTexture.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"

#include <utility>

TextureResource::~TextureResource()
{
	delete m_gpuTexture;
	m_gpuTexture = nullptr;
}

bool TextureResource::IsEmpty() const { return !m_image.IsValid() || !m_image->IsValid(); }

bool TextureResource::MoveFrom(Resource&& other)
{
	// 1) Validate type
	TextureResource* otherTex = dynamic_cast<TextureResource*>(&other);
	if (otherTex == nullptr)
	{
		return false;
	}

	// 2) Move CPU image data
	MoveBaseFrom(std::move(other));
	m_image = std::move(otherTex->m_image);

	// 3) Take ownership of the loaded GPU texture
	delete m_gpuTexture;
	m_gpuTexture = nullptr;
	std::swap(m_gpuTexture, otherTex->m_gpuTexture);

	return true;
}

bool TextureResource::InitGPUResources()
{
	// 1) Destroy old GPU texture
	delete m_gpuTexture;
	m_gpuTexture = nullptr;

	// 2) Create new GPU texture from CPU pixel data
	if (g_engine == nullptr || g_engine->m_renderer == nullptr || IsEmpty())
	{
		return false;
	}

	m_gpuTexture = g_engine->m_renderer->CreateGPUTexture(
		GetName().c_str(), m_image->GetDimensions(), m_image->GetChannels(), m_image->GetRawData());
	return m_gpuTexture != nullptr;
}

IntVec2 TextureResource::GetDimensions() const
{
	return m_image.IsValid() ? m_image->GetDimensions() : IntVec2::Zero;
}

int TextureResource::GetChannels() const { return m_image.IsValid() ? m_image->GetChannels() : 0; }
