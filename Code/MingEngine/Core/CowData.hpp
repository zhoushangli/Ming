#pragma once

#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

// A copy-on-write data block that can be shared by multiple owners.
// The whole block is one allocation, so the header is reachable by stepping one Header
// back from the element pointer that Data() hands out.
// e.g. Two CowData copied from each other share one block until DataW() is called.
// Layout: ┌───────────┬───────────┬───────────────┬───────────...
//         │ ref. size │ data size │ capacity size │ T[]
//         └───────────┴───────────┴───────────────┴───────────...
//         ↑ block start                           ↑ Data()
template <typename T>
class CowData
{
private:
	struct alignas(8) Header
	{
		uint32_t m_refCount = 1;
		uint32_t m_size     = 0;
		uint32_t m_capacity = 0;
	};

	// trivially copyable this type shouldn't manage memory itself
	// e.g. std::string is not trivially copyable
	static_assert(std::is_trivially_copyable_v<T>);
	static_assert(alignof(T) <= alignof(Header));
	static_assert(sizeof(Header) == 16);

public:
	CowData() = default;
	~CowData() { Release(); }

	CowData(CowData const& other) { Share(other); }
	CowData(CowData&& other) noexcept : m_data(std::exchange(other.m_data, nullptr)) {}
	CowData& operator=(CowData const& other)
	{
		if (this != &other)
		{
			Release();
			Share(other);
		}
		return *this;
	}
	CowData& operator=(CowData&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			m_data = std::exchange(other.m_data, nullptr);
		}
		return *this;
	}

	uint32_t Size() const { return m_data == nullptr ? 0 : GetHeader()->m_size; }
	uint32_t Capacity() const { return m_data == nullptr ? 0 : GetHeader()->m_capacity; }
	bool     IsEmpty() const { return Size() == 0; }

	T const* Data() const { return m_data; }
	T*       DataW()
	{
		return EnsureWritable(Size()) ? m_data : nullptr;
	}

	void SetData(T const* data, uint32_t count)
	{
		if (count == 0)
		{
			Clear();
			return;
		}

		ERR_FAIL_COND_MSG(data == nullptr, "CowData source is null");
		uintptr_t const source = reinterpret_cast<uintptr_t>(data);
		uintptr_t const start = reinterpret_cast<uintptr_t>(m_data);
		if (m_data != nullptr && source >= start && source - start < Size() * sizeof(T))
		{
			ERR_FAIL_COND_MSG((source - start) % sizeof(T) != 0 ||
				count > Size() - (source - start) / sizeof(T), "CowData source range is invalid");
		}

		Header* newHeader = Create(count, count);
		ERR_FAIL_COND_MSG(newHeader == nullptr, "CowData allocation failed");
		T* newData = reinterpret_cast<T*>(newHeader + 1);
		std::memcpy(newData, data, count * sizeof(T));
		Release();
		m_data = newData;
	}

	void Append(T value)
	{
		uint32_t const oldSize = Size();
		ERR_FAIL_COND_MSG(oldSize == std::numeric_limits<uint32_t>::max(), "CowData size overflow");
		uint32_t const newSize = oldSize + 1;
		if (!EnsureWritable(newSize))
		{
			return;
		}

		m_data[Size()]      = value;
		GetHeader()->m_size = newSize;
	}
	void Append(T const* data, uint32_t count)
	{
		if (count == 0)
		{
			return;
		}

		ERR_FAIL_COND_MSG(data == nullptr, "CowData source is null");

		uint32_t const oldSize = Size();

		if (oldSize > std::numeric_limits<uint32_t>::max() - count)
		{
			ERR_FAIL_MSG("CowData size overflow");
		}

		uintptr_t const source = reinterpret_cast<uintptr_t>(data);
		uintptr_t const start = reinterpret_cast<uintptr_t>(m_data);
		bool const isSelfAppend = m_data != nullptr && source >= start && source - start < oldSize * sizeof(T);
		uint32_t sourceIndex = 0;
		if (isSelfAppend)
		{
			sourceIndex = static_cast<uint32_t>((source - start) / sizeof(T));
			ERR_FAIL_COND_MSG((source - start) % sizeof(T) != 0 ||
				count > oldSize - sourceIndex, "CowData source range is invalid");
		}

		uint32_t const newSize = oldSize + count;
		if (!EnsureWritable(newSize))
		{
			return;
		}
		if (isSelfAppend)
		{
			data = m_data + sourceIndex;
		}
		std::memcpy(m_data + oldSize, data, count * sizeof(T));
		GetHeader()->m_size = newSize;
	}
	void Append(CowData const& other) { Append(other.Data(), other.Size()); }

	T* Resize(uint32_t size)
	{
		if (size == 0)
		{
			Clear();
			return nullptr;
		}

		if (!EnsureWritable(size))
		{
			return nullptr;
		}
		GetHeader()->m_size = size;
		return m_data;
	}
	void Clear()
	{
		Release();
		m_data = nullptr;
	}

private:
	Header* GetHeader() const { return reinterpret_cast<Header*>(reinterpret_cast<uint8_t*>(m_data) - sizeof(Header)); }

	bool EnsureWritable(uint32_t requiredCapacity)
	{
		if (m_data == nullptr && requiredCapacity == 0)
		{
			return true;
		}

		if (m_data != nullptr && GetHeader()->m_refCount == 1 && requiredCapacity <= Capacity())
		{
			return true;
		}

		uint32_t newCapacity = Capacity() > 2 ? Capacity() : 2;
		while (requiredCapacity > newCapacity)
		{
			if (newCapacity > std::numeric_limits<uint32_t>::max() / 2)
			{
				newCapacity = requiredCapacity;
				break;
			}

			newCapacity <<= 1;
		}

		Header* newHeader = Create(Size(), newCapacity);
		if (newHeader == nullptr)
		{
			ERR_PRINT("CowData allocation failed");
			return false;
		}
		T* newData = reinterpret_cast<T*>(newHeader + 1);

		if (Size() > 0)
		{
			std::memcpy(newData, m_data, Size() * sizeof(T));
		}
		Release();
		m_data = newData;
		return true;
	}

	static Header* Create(uint32_t size, uint32_t capacity)
	{
		if (capacity < size)
		{
			ERR_PRINT("CowData capacity is less than size");
			return nullptr;
		}

		if (capacity > (std::numeric_limits<size_t>::max() - sizeof(Header)) / sizeof(T))
		{
			ERR_PRINT("CowData allocation size overflow");
			return nullptr;
		}

		size_t const totalSize = sizeof(Header) + capacity * sizeof(T);
		void* const  block     = std::malloc(totalSize);
		if (block == nullptr)
		{
			return nullptr;
		}

		Header* header     = new (block) Header;
		header->m_size     = size;
		header->m_capacity = capacity;
		header->m_refCount = 1;

		return header;
	}

	void Share(CowData const& other)
	{
		m_data = other.m_data;
		if (m_data != nullptr)
		{
			GetHeader()->m_refCount++;
		}
	}
	void Release()
	{
		if (m_data == nullptr)
		{
			return;
		}

		Header* header = GetHeader();
		if (--header->m_refCount == 0)
		{
			std::free(header);
		}
		m_data = nullptr;
	}

private:
	T* m_data = nullptr;
};
