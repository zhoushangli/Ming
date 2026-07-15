#pragma once

#include "MingEngine/Core/Image.hpp"
#include "MingEngine/Core/Object/Resource.hpp"

class GPUTexture;

class TextureResource : public Resource
{
	MCLASS(TextureResource, Resource)

public:
	TextureResource()                                       = default;
	TextureResource(TextureResource const& copy)            = delete;
	TextureResource& operator=(TextureResource const& copy) = delete;
	~TextureResource();

	bool IsEmpty() const;
	bool MoveFrom(Resource&& other) override;

	bool InitGPUResources();

	GPUTexture* GetGPUTexture() const { return m_gpuTexture; }
	Ref<Image>  GetImage() const { return m_image; }
	IntVec2     GetDimensions() const;
	int         GetChannels() const;

protected:
	static void BindMethods() {}

public:
	Ref<Image> m_image;

private:
	GPUTexture* m_gpuTexture = nullptr;
};
