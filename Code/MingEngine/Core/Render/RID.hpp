#pragma once

#include <cstdint>
#include <vector>

struct RID
{
public:
	RID();
	RID(uint32_t uid, uint32_t index);

	bool     IsValid() const;
	uint32_t GetIndex() const;
	uint32_t GetUID() const;
	bool     operator==(RID const& other) const;
	bool     operator!=(RID const& other) const;
	bool     operator<(RID const& other) const;

	static const RID          Invalid;
	static constexpr uint32_t kMaxUID   = 0xfffffffeu;
	static constexpr uint32_t kMaxIndex = 0xffffffffu;

private:
	uint64_t m_data = 0xffffffffffffffffull;
};

// By conclude to a single base class
// we can have a single static s_nextUID for all RIDOwner<T> instances.
class RIDOwnerBase
{
protected:
	inline static int s_nextUID = 0;
};

template <typename T>
class RIDOwner : public RIDOwnerBase
{
private:
	struct Entry
	{
		T*  m_value = nullptr;
		RID m_rid   = RID::Invalid;
	};

public:
	RIDOwner() = default;
	~RIDOwner()
	{
		for (Entry& entry : m_entries)
		{
			if (entry.m_value)
			{
				delete entry.m_value;
			}
		};

		m_entries.clear();
	}

	RIDOwner(RIDOwner const&)            = delete;
	RIDOwner& operator=(RIDOwner const&) = delete;

	RID CreateRID()
	{
		size_t index = 0;
		for (; index < m_entries.size(); ++index)
		{
			if (m_entries[index].m_value == nullptr)
			{
				break;
			}
		}

		RID rid   = RID(s_nextUID, static_cast<uint32_t>(index));
		T*  value = new T();

		Entry entry;
		entry.m_value = value;
		entry.m_rid   = rid;

		if (index == m_entries.size())
		{
			m_entries.push_back(entry);
		}
		else
		{
			m_entries[index] = entry;
		}

		s_nextUID = s_nextUID == RID::kMaxUID ? 1u : s_nextUID + 1u;

		return rid;
	}
	RID CreateRID(T const& value)
	{
		size_t index = 0;
		for (; index < m_entries.size(); ++index)
		{
			if (m_entries[index].m_value == nullptr)
			{
				break;
			}
		}

		RID rid = RID(s_nextUID, static_cast<uint32_t>(index));

		Entry entry;
		entry.m_value = new T(value);
		entry.m_rid   = rid;

		if (index == m_entries.size())
		{
			m_entries.push_back(entry);
		}
		else
		{
			m_entries[index] = entry;
		}

		s_nextUID = s_nextUID == RID::kMaxUID ? 1u : s_nextUID + 1u;

		return rid;
	}

	T* GetOrNull(RID rid)
	{
		if (!rid.IsValid())
		{
			return nullptr;
		}

		uint32_t const index = rid.GetIndex();
		if (index >= m_entries.size())
		{
			return nullptr;
		}

		Entry const& entry = m_entries[index];
		if (entry.m_rid != rid)
		{
			return nullptr;
		}

		return entry.m_value;
	}
	T const* GetOrNull(RID rid) const
	{
		if (!rid.IsValid())
		{
			return nullptr;
		}

		uint32_t const index = rid.GetIndex();
		if (index >= m_entries.size())
		{
			return nullptr;
		}

		Entry const& entry = m_entries[index];
		if (entry.m_rid != rid)
		{
			return nullptr;
		}

		return entry.m_value;
	}
	std::vector<RID> GetRIDList() const
	{
		std::vector<RID> rids;
		for (Entry const& entry : m_entries)
		{
			if (entry.m_value != nullptr)
			{
				rids.push_back(entry.m_rid);
			}
		}

		return rids;
	}

	bool Exist(RID rid) const
	{
		T const* value = GetOrNull(rid);
		return value != nullptr;
	}
	void Free(RID rid)
	{
		T* value = GetOrNull(rid);
		if (value == nullptr)
		{
			return;
		}

		uint32_t const index = rid.GetIndex();
		if (index < m_entries.size())
		{
			m_entries[index].m_value = nullptr;
			m_entries[index].m_rid   = RID::Invalid;
		}

		delete value;
	}

private:
	std::vector<Entry> m_entries;
};
