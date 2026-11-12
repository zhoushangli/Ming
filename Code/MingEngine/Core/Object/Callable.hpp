#pragma once

#include "MingEngine/Core/Object/CallError.hpp"
#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Core/Object/Variant.hpp"
#include "MingEngine/Core/String.hpp"

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class Object;
struct ManagedCallable;

class Callable
{
public:
	using CallError         = ::CallError;
	using InvokeDelegate    = void (*)(void*, void*, Variant const* const*, int32_t, Variant*, CallError*);
	using CompareDelegates = int32_t (*)(void*, void*);
	using ReleaseDelegate   = void (*)(void*);

public:
	Callable() = default;
	Callable(Object const* target, String const& methodName);
	~Callable();

	Callable(Callable const& other);
	Callable(Callable&& other) noexcept;
	Callable& operator=(Callable const& other);
	Callable& operator=(Callable&& other) noexcept;

	// Retain a real delegate and its typed trampoline in shared native state.
	// e.g. FromManaged(nullptr, handle, trampoline, invoke, compare, release) can represent a closure.
	static Callable FromManaged(Object const* target, void* handle, void* trampoline,
		InvokeDelegate invoke, CompareDelegates compare, ReleaseDelegate release);

	static void InvalidateManagedCallables();
	static bool IsInvokingManagedCallable();

	Object*            GetTarget() const;
	ObjectID           GetTargetID() const { return m_targetID; }
	String const&      GetMethodName() const { return m_methodName; }
	bool               IsValid() const;
	bool               IsManaged() const { return m_managed != nullptr; }

	bool operator==(Callable const& other) const;
	bool operator!=(Callable const& other) const;

	// Pack arguments for this invocation and return the method's result; report call errors to the log.
	// e.g. Callable(player, "SetHealth").Call(80) invokes player->SetHealth(80).
	template <typename... Args, std::enable_if_t<(std::is_constructible_v<Variant, Args&&> && ...), int> = 0>
	Variant Call(Args&&... args) const
	{
		std::vector<Variant> arguments{ Variant(std::forward<Args>(args))... };
		return Call(static_cast<std::vector<Variant> const&>(arguments));
	}

	// Invoke with an existing argument list; failures return an empty Variant and log or fill the supplied error.
	// e.g. Call({ Variant(80) }, error) distinguishes a successful void method from an invalid call.
	Variant Call(std::vector<Variant> const& arguments) const;
	Variant Call(std::vector<Variant> const& arguments, CallError& error) const;

private:
	void ReleaseManaged();

private:
	ObjectID         m_targetID = ObjectID::Invalid;
	String           m_methodName;
	ManagedCallable* m_managed = nullptr;
};
