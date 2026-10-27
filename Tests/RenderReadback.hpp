#pragma once

#include "MingEngine/Engine/Render/GPUTexture.hpp"
#include "MingEngine/Core/Render/Color.hpp"
#include <d3d11.h>
#include <wrl/client.h>
#include <filesystem>
#include <fstream>
#include <stdexcept>

inline Color ReadPixel(GPUTexture* texture, unsigned x, unsigned y)
{
	using Microsoft::WRL::ComPtr;
	ComPtr<ID3D11Resource> resource;
	texture->GetShaderResourceView()->GetResource(&resource);
	ComPtr<ID3D11Texture2D> source;
	resource.As(&source);
	ComPtr<ID3D11Device> device;
	source->GetDevice(&device);
	ComPtr<ID3D11DeviceContext> context;
	device->GetImmediateContext(&context);
	D3D11_TEXTURE2D_DESC description;
	source->GetDesc(&description);
	description.Usage          = D3D11_USAGE_STAGING;
	description.BindFlags      = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	description.MiscFlags      = 0;
	ComPtr<ID3D11Texture2D> staging;
	if (FAILED(device->CreateTexture2D(&description, nullptr, &staging)))
	{
		throw std::runtime_error("Could not allocate GPU verification readback.");
	}

	context->CopyResource(staging.Get(), source.Get());
	D3D11_MAPPED_SUBRESOURCE mapped;
	if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
	{
		throw std::runtime_error("Could not read GPU verification pixels.");
	}

	auto const* pixel = static_cast<uint8_t const*>(mapped.pData) + y * mapped.RowPitch + x * 4;
	Color const result(pixel[0], pixel[1], pixel[2], pixel[3]);
	context->Unmap(staging.Get(), 0);
	return result;
}

inline void SaveRenderedFrame(GPUTexture* texture, std::filesystem::path const& path)
{
	using Microsoft::WRL::ComPtr;
	ComPtr<ID3D11Resource> resource;
	texture->GetShaderResourceView()->GetResource(&resource);
	ComPtr<ID3D11Texture2D> source;
	resource.As(&source);
	ComPtr<ID3D11Device> device;
	source->GetDevice(&device);
	ComPtr<ID3D11DeviceContext> context;
	device->GetImmediateContext(&context);
	D3D11_TEXTURE2D_DESC description;
	source->GetDesc(&description);
	description.Usage          = D3D11_USAGE_STAGING;
	description.BindFlags      = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	description.MiscFlags      = 0;
	ComPtr<ID3D11Texture2D> staging;
	if (FAILED(device->CreateTexture2D(&description, nullptr, &staging)))
	{
		throw std::runtime_error("Cannot allocate screenshot readback.");
	}

	context->CopyResource(staging.Get(), source.Get());
	D3D11_MAPPED_SUBRESOURCE mapped;
	if (FAILED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
	{
		throw std::runtime_error("Cannot read rendered screenshot.");
	}

	std::ofstream output(path, std::ios::binary);
	output << "P6\n" << description.Width << " " << description.Height << "\n255\n";
	for (UINT y = 0; y < description.Height; ++y)
	{
		auto const* pixels = static_cast<uint8_t const*>(mapped.pData) + y * mapped.RowPitch;
		for (UINT x = 0; x < description.Width; ++x)
		{
			output.write(reinterpret_cast<char const*>(pixels + x * 4), 3);
		}
	}

	context->Unmap(staging.Get(), 0);
}
