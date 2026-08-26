#include "MingEngine/Core/Object/ObjectID.hpp"

const ObjectID ObjectID::Invalid(0xffffffffu, 0xffffffffu);

ObjectID::ObjectID() { m_data = 0xffffffffffffffffull; }

ObjectID::ObjectID(uint32_t uid, uint32_t index)
{
	m_data = (static_cast<uint64_t>(uid) << 32) | static_cast<uint64_t>(index);
}

bool ObjectID::IsValid() const { return *this != Invalid; }

uint32_t ObjectID::GetIndex() const { return static_cast<uint32_t>(m_data & 0xffffffffull); }

uint32_t ObjectID::GetUID() const { return static_cast<uint32_t>((m_data >> 32) & 0xffffffffull); }

bool ObjectID::operator==(ObjectID const& other) const { return m_data == other.m_data; }

bool ObjectID::operator!=(ObjectID const& other) const { return !(*this == other); }

bool ObjectID::operator<(ObjectID const& other) const { return m_data < other.m_data; }
