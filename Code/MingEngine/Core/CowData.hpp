#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

// A copy-on-write data block that can be shared by multiple owners.
// The whole block is one allocation, so the header is reachable by stepping one Header
// back from the element pointer that Data() hands out.
// e.g. Two CowData copied from each other share one block until DataW() is called.
// Layout: ┌────────────┬────────────┬───────────...
//         │ ref. count │ data size  │ T[]
//         └────────────┴────────────┴───────────...
//         ↑ block start              ↑ Data()
template <typename T>
class CowData
{
private:
	struct alignas(8) Header
	{
		uint32_t m_refCount = 1;
		uint32_t m_size     = 0;
	};

	static_assert(std::is_trivially_copyable_v<T>, "CowData only copies raw bytes, use trivial types.");
	static_assert(alignof(T) <= alignof(Header), "The elements must not need more alignment than the header.");

public:
	CowData() = default;
	~CowData() { Release(); }

	CowData(CowData const& other) { Share(other); }
	CowData(CowData&& other) noexcept : m_data(other.m_data) { other.m_data = nullptr; }

	CowData& operator=(CowData const& other)
	{
		if (m_data != other.m_data)
		{
			Release();
			Share(other);
		}
		return *this;
	}

	CowData& operator=(CowData&& other) noexcept
	{
		std::swap(m_data, other.m_data);
		return *this;
	}

	uint32_t Size() const { return m_data == nullptr ? 0u : GetHeader()->m_size; }
	bool     IsEmpty() const { return Size() == 0; }
	T const* Data() const { return m_data; }

	// Hand out a writable pointer and break the sharing first, so every other owner keeps
	// the content it had.
	// e.g. DataW()[0] = U'A' leaves the copies of this data untouched.
	T* DataW()
	{
		CopyIfShared();
		return m_data;
	}

	// Replace the content with a copy of the given elements and own a fresh block.
	// e.g. Assign(codes, 4) drops the old block instead of writing into it.
	void Assign(T const* data, uint32_t size)
	{
		Header* header = nullptr;
		if (size > 0 && data != nullptr)
		{
			header = Allocate(size);
			std::memcpy(header + 1, data, sizeof(T) * size);
		}

		Release();
		m_data = header == nullptr ? nullptr : reinterpret_cast<T*>(header + 1);
	}

	// Own a new block of the given element count and hand out a writable pointer to it.
	// The first min(old size, new size) elements keep the values they had.
	// e.g. Resize(6) on a 4 element block keeps 4 values and leaves 2 elements uninitialized.
	T* Resize(uint32_t size)
	{
		if (size == 0)
		{
			Release();
			return nullptr;
		}

		Header* header = Allocate(size);
		if (m_data != nullptr)
		{
			uint32_t const keptSize = size < Size() ? size : Size();
			std::memcpy(header + 1, m_data, sizeof(T) * keptSize);
		}

		Release();
		m_data = reinterpret_cast<T*>(header + 1);

		return m_data;
	}

	void Clear() { Release(); }

private:
	Header* GetHeader() const { return reinterpret_cast<Header*>(m_data) - 1; }

	// Allocate one block that holds the header and the elements with a single owner.
	// e.g. Allocate(4) returns room for 4 elements and a reference count of 1.
	static Header* Allocate(uint32_t size)
	{
		Header* header     = static_cast<Header*>(::operator new(sizeof(Header) + sizeof(T) * size));
		header->m_refCount = 1;
		header->m_size     = size;
		return header;
	}

	// Share the data block and increase the reference count
	void Share(CowData const& other)
	{
		m_data = other.m_data;
		if (m_data != nullptr)
		{
			GetHeader()->m_refCount++;
		}
	}

	// Drop this reference to the block and free the block when it was the last one.
	// e.g. Release() on the only owner of a 4 element block frees the whole allocation.
	void Release()
	{
		if (m_data == nullptr)
		{
			return;
		}

		// The header sits right before the elements, so freeing it frees the whole block
		// that Allocate() requested with a single ::operator new call.
		Header* header = GetHeader();

		--header->m_refCount;
		if (header->m_refCount == 0)
		{
			::operator delete(header);
		}

		m_data = nullptr;
	}

	// If you are about to write into the data
	// We just copy ourself and release the old block
	void CopyIfShared()
	{
		if (m_data == nullptr || GetHeader()->m_refCount == 1)
		{
			return;
		}

		uint32_t const size = GetHeader()->m_size;
		Header*        copy = Allocate(size);
		std::memcpy(copy + 1, m_data, sizeof(T) * size);
		Release();
		m_data = reinterpret_cast<T*>(copy + 1);
	}

private:
	T* m_data = nullptr;
};