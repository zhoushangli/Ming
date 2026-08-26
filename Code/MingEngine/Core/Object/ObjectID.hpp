#pragma once

#include <cstdint>

struct ObjectID
{
public:
	ObjectID();
	ObjectID(uint32_t uid, uint32_t index);

	bool     IsValid() const;
	uint32_t GetIndex() const;
	uint32_t GetUID() const;
	bool     operator==(ObjectID const& other) const;
	bool     operator!=(ObjectID const& other) const;
	bool     operator<(ObjectID const& other) const;

	static const ObjectID     Invalid;
	static constexpr uint32_t kMaxObjectUID   = 0xfffffffeu;
	static constexpr uint32_t kMaxObjectIndex = 0xffffffffu;

private:
	uint64_t m_data = 0xffffffffffffffffull;
};

static_assert(sizeof(ObjectID) == sizeof(uint64_t));
