#pragma once

class ManagedGCHandle
{
public:
	ManagedGCHandle() = default;

	explicit ManagedGCHandle(void* value) : m_value(value) {}

	~ManagedGCHandle();

	ManagedGCHandle(ManagedGCHandle const&)            = delete;
	ManagedGCHandle& operator=(ManagedGCHandle const&) = delete;

	ManagedGCHandle(ManagedGCHandle&& other);

	ManagedGCHandle& operator=(ManagedGCHandle&& other);

	bool  IsValid() const { return m_value != nullptr; }
	void* GetValue() const { return m_value; }

	void Release();
	void* Detach()
	{
		void* value = m_value;
		m_value = nullptr;
		return value;
	}

private:
	void* m_value = nullptr;
};