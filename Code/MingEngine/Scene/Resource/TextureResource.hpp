#pragma once

#include "MingEngine/Core/IO/Image.hpp"
#include "MingEngine/Core/IO/Resource.hpp"
#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Engine/Render/RenderTypes.hpp"

class TextureResource : public Resource
{
	MCLASS(TextureResource, Resource)

public:
	TextureResource() = default;
	~TextureResource();

	TextureResource(TextureResource const& copy)            = delete;
	TextureResource& operator=(TextureResource const& copy) = delete;

	bool IsEmpty() const;
	bool CopyFrom(Resource&& other) override;

	bool InitGPUResources();

	RID        GetTextureRID() const { return m_textureRID; }
	Ref<Image> GetImage() const { return m_image; }

	IntVec2 GetDimensions() const;
	int     GetChannels() const;

protected:
	static void BindMethods() {}

public:
	Ref<Image>  m_image;
	SamplerMode m_samplerMode = SamplerMode::POINT_CLAMP;

private:
	// Texture RID on the RenderServer; RID::Invalid means the image has no GPU data yet.
	// e.g. InitGPUResources() registers it and MeshCreate() copies this RID into MeshData
	RID m_textureRID = RID::Invalid;
};
