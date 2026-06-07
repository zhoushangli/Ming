#include "MingEngine/Scene/Core/NodeHandle.hpp"

const NodeHandle NodeHandle::Invalid(0x0000ffffu, 0x0000ffffu);

NodeHandle::NodeHandle() { m_data = 0xffffffffu; }

NodeHandle::NodeHandle(unsigned int uid, unsigned int index)
{
	m_data = ((uid & 0x0000ffffu) << 16) | (index & 0x0000ffffu);
}

bool NodeHandle::IsValid() const { return *this != Invalid; }

unsigned int NodeHandle::GetIndex() const { return m_data & 0x0000ffffu; }

unsigned int NodeHandle::GetUID() const { return (m_data >> 16) & 0x0000ffffu; }

bool NodeHandle::operator==(NodeHandle const& other) const { return m_data == other.m_data; }

bool NodeHandle::operator!=(NodeHandle const& other) const { return !(*this == other); }

bool NodeHandle::operator<(NodeHandle const& other) const { return m_data < other.m_data; }
