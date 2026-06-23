#pragma once

#include "MingEngine/Core/Object/Object.hpp"

class RefCounted : public Object
{
public:
	RefCounted()          = default;
	virtual ~RefCounted() = default;

	bool InitRef();
	bool Ref();
	bool Unref();

	int GetRefCount() const;

private:
	int m_refCount = 0;
};

template <typename T>
class Ref
{
	static_assert(std::is_base_of_v<RefCounted, T>, "Ref<T> can only be used with classes derived from RefCounted.");

public:
	Ref() = default;
	explicit Ref(T* ptr) { RefPointer(ptr); }
	Ref(Ref<T> const& other) { RefPointer(other.reference); }
	Ref(Ref<T>&& other) noexcept
	{
		RefPointer(other.reference);
		other.RefPointer(nullptr);
	}
	~Ref() { RefPointer(nullptr); }

	Ref& operator=(Ref<T> const& other)
	{
		if (this != &other)
		{
			RefPointer(other.reference);
		}
		return *this;
	}

	Ref& operator=(Ref<T>&& other) noexcept
	{
		if (this != &other)
		{
			RefPointer(other.reference);
			other.RefPointer(nullptr);
		}
		return *this;
	}

	T* Get() const { return reference; }
	T* operator->() { return reference; }
	T& operator*() { return *reference; }

	bool IsValid() const { return reference != nullptr; }
	bool IsNull() const { return reference == nullptr; }

	bool operator==(const Ref<T>& other) const { return reference == other.reference; }
	bool operator!=(const Ref<T>& other) const { return reference != other.reference; }
	bool operator==(const T* ptr) const { return reference == ptr; }
	bool operator!=(const T* ptr) const { return reference != ptr; }

private:
	void RefPointer(T* ptr)
	{
		if (ptr == reference)
		{
			return;
		}

		// 1) Unref the current reference if it exists
		if (reference != nullptr)
		{
			if (reference->Unref())
			{
				delete reference;
				reference = nullptr;
			}
		}

		reference = ptr;

		// 2) Ref the new reference if it exists
		if (reference)
		{
			if (reference->GetRefCount() == 0)
			{
				reference->InitRef();
			}
			else
			{
				reference->Ref();
			}
		}
	}

private:
	T* reference = nullptr;
};