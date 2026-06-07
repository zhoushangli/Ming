#include "MingEngine/Engine/Render/ConstantBuffer.hpp"

#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"

#include <d3d11.h>

ConstantBuffer::ConstantBuffer(ID3D11Device* device, size_t size) : m_device(device), m_size(size)
{
    Create();
}

ConstantBuffer::~ConstantBuffer()
{
    if (m_buffer)
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }
}

void ConstantBuffer::Create()
{
    if (!m_device) return;

    if (m_buffer)
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = (UINT)m_size;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    const HRESULT hr = m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);

    GUARANTEE_OR_DIE(SUCCEEDED(hr), "ConstantBuffer::Create failed to create D3D11 constant buffer");
}
