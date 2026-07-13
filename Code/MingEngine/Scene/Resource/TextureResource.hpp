#pragma once

#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Object/Resource.hpp"

#include <cstdint>
#include <string>
#include <vector>

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

	void InitGPUResources();

	GPUTexture* GetGPUTexture() const { return m_gpuTexture; }
	IntVec2     GetDimensions() const { return m_dimensions; }
	int         GetChannels() const { return m_channels; }

protected:
	static void BindMethods() {}

public:
	std::string          m_format   = "RGBA8";
	int                  m_channels = 4;
	IntVec2              m_dimensions;
	std::vector<uint8_t> m_pixels;

private:
	GPUTexture* m_gpuTexture = nullptr;
};
