#pragma once


class ManagedGCHandle
{
public:
	ManagedGCHandle() = default;

	explicit ManagedGCHandle(void* value) : m_value(value) {}

	ManagedGCHandle(ManagedGCHandle const&)            = delete;
	ManagedGCHandle& operator=(ManagedGCHandle const&) = delete;

	ManagedGCHandle(ManagedGCHandle&& other);

	ManagedGCHandle& operator=(ManagedGCHandle&& other);

	~ManagedGCHandle();

	bool IsValid() const { return m_value != nullptr; }

	void* GetValue() const { return m_value; }

	void Release();

private:
	void* m_value = nullptr;
};