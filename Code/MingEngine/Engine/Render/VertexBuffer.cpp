#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include <d3d11.h>

VertexBuffer::VertexBuffer(ID3D11Device* device, unsigned int size, unsigned int stride)
    : m_device(device)
    , m_size(size)
    , m_stride(stride)
    , m_buffer(nullptr)
{
    Create();
}

VertexBuffer::~VertexBuffer()
{
    if (m_buffer) 
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }
}

void VertexBuffer::Create()
{
    if (!m_device) return;

    if (m_buffer) 
    {
        m_buffer->Release();
        m_buffer = nullptr;
    }

    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.ByteWidth = m_size;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = m_stride;

    m_device->CreateBuffer(&bufferDesc, nullptr, &m_buffer);
}

void VertexBuffer::Resize(unsigned int size)
{
    m_size = size;
    Create();
}

unsigned int VertexBuffer::GetSize()
{
    return m_size;
}

unsigned int VertexBuffer::GetStride()
{
    return m_stride;
}

unsigned int VertexBuffer::GetCount()
{
    return m_stride ? (m_size / m_stride) : 0;
}
