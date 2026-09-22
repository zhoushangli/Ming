#include "MingEngine/Core/Render/RID.hpp"

const RID RID::Invalid(0xffffffffu, 0xffffffffu);

RID::RID() { m_data = 0xffffffffffffffffull; }

RID::RID(uint32_t uid, uint32_t index) { m_data = (static_cast<uint64_t>(uid) << 32) | static_cast<uint64_t>(index); }

bool RID::IsValid() const { return *this != Invalid; }

uint32_t RID::GetIndex() const { return static_cast<uint32_t>(m_data & 0xffffffffull); }

uint32_t RID::GetUID() const { return static_cast<uint32_t>((m_data >> 32) & 0xffffffffull); }

bool RID::operator==(RID const& other) const { return m_data == other.m_data; }

bool RID::operator!=(RID const& other) const { return !(*this == other); }

bool RID::operator<(RID const& other) const { return m_data < other.m_data; }
