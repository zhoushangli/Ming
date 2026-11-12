#include "MingEngine/Core/Object/Callable.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/Object.hpp"

#include <memory>
#include <unordered_set>

// Share one delegate handle between Callable owners on the engine thread.
// e.g. A signal snapshot retains this state after the live connection is removed.
struct ManagedCallable
{
	uint32_t m_refCount = 1;
	void* m_handle = nullptr;
	void* m_trampoline = nullptr;
	Callable::InvokeDelegate m_invoke = nullptr;
	Callable::CompareDelegates m_compare = nullptr;
	Callable::ReleaseDelegate m_release = nullptr;
};

namespace
{
std::unordered_set<ManagedCallable*> s_managedCallables;
uint32_t s_activeManagedCalls = 0;
}

Callable::Callable(Object const* target, String const& methodName) : m_methodName(methodName)
{
	static_assert(std::is_standard_layout_v<Callable>);
	static_assert(sizeof(Callable) == 24 && alignof(Callable) == 8);
	static_assert(offsetof(Callable, m_targetID) == 0 && offsetof(Callable, m_methodName) == 8);
	static_assert(offsetof(Callable, m_managed) == 16);

	if (target != nullptr)
	{
		m_targetID = target->GetObjectID();
	}
}

Callable::~Callable()
{
	ReleaseManaged();
}

Callable::Callable(Callable const& other)
	: m_targetID(other.m_targetID), m_methodName(other.m_methodName), m_managed(other.m_managed)
{
	if (m_managed != nullptr)
	{
		++m_managed->m_refCount;
	}
}

Callable::Callable(Callable&& other) noexcept
	: m_targetID(std::exchange(other.m_targetID, ObjectID::Invalid)),
	  m_methodName(std::move(other.m_methodName)), m_managed(std::exchange(other.m_managed, nullptr))
{
}

Callable& Callable::operator=(Callable const& other)
{
	if (this != &other)
	{
		Callable copy(other);
		*this = std::move(copy);
	}

	return *this;
}

Callable& Callable::operator=(Callable&& other) noexcept
{
	if (this != &other)
	{
		ReleaseManaged();
		m_targetID = std::exchange(other.m_targetID, ObjectID::Invalid);
		m_methodName = std::move(other.m_methodName);
		m_managed = std::exchange(other.m_managed, nullptr);
	}

	return *this;
}

Callable Callable::FromManaged(Object const* target, void* handle, void* trampoline,
	InvokeDelegate invoke, CompareDelegates compare, ReleaseDelegate release)
{
	auto state = std::make_unique<ManagedCallable>();
	state->m_handle = handle;
	state->m_trampoline = trampoline;
	state->m_invoke = invoke;
	state->m_compare = compare;
	state->m_release = release;
	s_managedCallables.insert(state.get());

	Callable result;
	result.m_targetID = target != nullptr ? target->GetObjectID() : ObjectID::Invalid;
	result.m_managed = state.release();
	return result;
}

void Callable::ReleaseManaged()
{
	ManagedCallable* state = std::exchange(m_managed, nullptr);
	if (state == nullptr || --state->m_refCount != 0)
	{
		return;
	}

	s_managedCallables.erase(state);
	if (state->m_handle != nullptr)
	{
		state->m_release(state->m_handle);
	}

	delete state;
}

void Callable::InvalidateManagedCallables()
{
	// 1) Clear every native entry before releasing its collectible managed references
	for (ManagedCallable* state : s_managedCallables)
	{
		void* handle = std::exchange(state->m_handle, nullptr);
		ReleaseDelegate release = std::exchange(state->m_release, nullptr);
		state->m_trampoline = nullptr;
		state->m_invoke = nullptr;
		state->m_compare = nullptr;
		if (handle != nullptr)
		{
			release(handle);
		}
	}
}

bool Callable::IsInvokingManagedCallable()
{
	return s_activeManagedCalls != 0;
}

Object* Callable::GetTarget() const
{
	return ObjectDatabase::GetInstance(m_targetID);
}

bool Callable::IsValid() const
{
	if (m_managed != nullptr)
	{
		Object* target = GetTarget();
		return m_managed->m_handle != nullptr
			&& (!m_targetID.IsValid() || (target != nullptr && !target->IsPreDeleting()));
	}

	Object* target = GetTarget();
	if (target == nullptr || target->IsPreDeleting())
	{
		return false;
	}

	return target->HasMethod(m_methodName.ToUtf8());
}

bool Callable::operator==(Callable const& other) const
{
	if (m_managed != nullptr || other.m_managed != nullptr)
	{
		if (m_managed == nullptr || other.m_managed == nullptr)
		{
			return false;
		}

		return m_managed == other.m_managed
			|| (m_managed->m_handle != nullptr && other.m_managed->m_handle != nullptr
				&& m_managed->m_compare(m_managed->m_handle, other.m_managed->m_handle) != 0);
	}

	return m_targetID == other.m_targetID && m_methodName == other.m_methodName;
}

bool Callable::operator!=(Callable const& other) const
{
	return !(*this == other);
}

Variant Callable::Call(std::vector<Variant> const& arguments) const
{
	Callable owner(*this);
	CallError error;
	Variant   result = owner.Call(arguments, error);
	if (error.m_error != CallError::Error::Ok)
	{
		ERR_PRINT("Callable: failed to invoke '" + (owner.IsManaged() ? std::string("Delegate::Invoke") : owner.m_methodName.ToUtf8())
			+ "'. Use Call(arguments, error) for details.");
	}

	return result;
}

Variant Callable::Call(std::vector<Variant> const& arguments, CallError& error) const
{
	error = {};
	if (m_managed != nullptr)
	{
		if (!IsValid())
		{
			error.m_error = CallError::Error::InvalidTarget;
			return {};
		}

		// 1) Keep this state alive even if the callback destroys its original owner
		Callable owner(*this);
		std::vector<Variant const*> pointers;
		pointers.reserve(arguments.size());
		for (Variant const& argument : arguments)
		{
			pointers.push_back(&argument);
		}

		Variant result;
		++s_activeManagedCalls;
		owner.m_managed->m_invoke(owner.m_managed->m_handle, owner.m_managed->m_trampoline,
			pointers.data(), static_cast<int32_t>(pointers.size()), &result, &error);
		--s_activeManagedCalls;
		return result;
	}

	// 1) Resolve the target each time without retaining the object
	Object* target = ObjectDatabase::GetInstance(m_targetID);
	if (target == nullptr)
	{
		error.m_error = CallError::Error::InvalidTarget;
		return {};
	}

	return target->Call(m_methodName.ToUtf8(), arguments, error);
}
