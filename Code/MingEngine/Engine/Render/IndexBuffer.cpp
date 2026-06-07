#include "MingEngine/Engine/Render/IndexBuffer.hpp"

#include <d3d11.h>

IndexBuffer::IndexBuffer(ID3D11Device* device, unsigned int size) : m_device(device), m_size(size), m_buffer(nullptr)
{
    Create();
}

IndexBuffer::~IndexBuffer()
{
    if (m_buffer)
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }
}

void IndexBuffer::Resize(unsigned int size)
{
    m_size = size;
    Create();
}

void IndexBuffer::Create()
{
    if (m_buffer)
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = m_size;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;

    m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
}

unsigned int IndexBuffer::GetSize()
{
    return m_size;
}

unsigned int IndexBuffer::GetStride()
{
    return sizeof(unsigned int);
}

unsigned int IndexBuffer::GetCount()
{
    return GetSize() / GetStride();
}
