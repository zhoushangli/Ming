#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"

#include <utility>

TextureResource::~TextureResource()
{
	// The registration is released while the RenderServer still owns the GPUTexture.
	RenderServer* server = (g_engine != nullptr) ? g_engine->m_renderServer : nullptr;
	if (server != nullptr && m_textureRID.IsValid())
	{
		server->TextureFree(m_textureRID);
		m_textureRID = RID::Invalid;
	}
}

bool TextureResource::IsEmpty() const { return !m_image.IsValid() || !m_image->IsValid(); }

bool TextureResource::CopyFrom(Resource&& other)
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

	// 3) Hand the loaded GPU texture to the registration of this resource.
	//    The other resource gives up its RID so it cannot free what this one keeps.
	RenderServer* server = (g_engine != nullptr) ? g_engine->m_renderServer : nullptr;
	if (server != nullptr)
	{
		if (m_textureRID.IsValid())
		{
			server->TextureRefresh(m_textureRID, *this);
		}
		else
		{
			m_textureRID = server->TextureCreate(*this);
		}

		if (otherTex->m_textureRID.IsValid())
		{
			server->TextureFree(otherTex->m_textureRID);
			otherTex->m_textureRID = RID::Invalid;
		}
	}

	return true;
}

bool TextureResource::InitGPUResources()
{
	// 1) Nothing can be uploaded while the RenderServer is down.
	// 2) Register the image once and then keep that RID stable across reloads.
	RenderServer* server = (g_engine != nullptr) ? g_engine->m_renderServer : nullptr;
	if (server == nullptr || IsEmpty())
	{
		return false;
	}

	if (m_textureRID.IsValid())
	{
		server->TextureRefresh(m_textureRID, *this);
	}
	else
	{
		m_textureRID = server->TextureCreate(*this);
	}

	return m_textureRID.IsValid();
}

IntVec2 TextureResource::GetDimensions() const { return m_image.IsValid() ? m_image->GetDimensions() : IntVec2::Zero; }

int TextureResource::GetChannels() const { return m_image.IsValid() ? m_image->GetChannels() : 0; }
