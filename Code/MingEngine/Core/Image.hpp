#pragma once

#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct Rgba8;

class Image : public RefCounted
{
	MCLASS(Image, RefCounted)

public:
	Image() = default;

	// Load encoded image bytes from a physical file, retain them, and decode RGBA8 pixels.
	// e.g. image->LoadFromFile("C:/Textures/Board.png")
	bool LoadFromFile(std::string const& physicalPath);

	// Load encoded image data, retain the source bytes, and decode RGBA8 pixels.
	// e.g. image->LoadFromMemory(std::move(pngData))
	bool LoadFromMemory(std::vector<uint8_t> encodedData);

	void Clear();
	bool IsValid() const;
	bool HasEncodedData() const;

	Rgba8 GetColorAt(int x, int y) const;

	IntVec2                    GetDimensions() const { return m_dimensions; }
	int                        GetChannels() const { return m_channels; }
	uint8_t const*             GetRawData() const { return m_pixels.empty() ? nullptr : m_pixels.data(); }
	size_t                     GetDataSize() const { return m_pixels.size(); }
	std::vector<uint8_t> const& GetEncodedData() const { return m_encodedData; }

protected:
	static void BindMethods() {}

private:
	IntVec2              m_dimensions = IntVec2::Zero;
	int                  m_channels   = 4;
	std::vector<uint8_t> m_encodedData;
	std::vector<uint8_t> m_pixels;
};
