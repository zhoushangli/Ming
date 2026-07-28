#pragma once

#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

class MethodBind
{
public:
	virtual ~MethodBind() = default;

	virtual Variant Invoke(Object* object, std::vector<Variant> const& arguments) const = 0;
};

template <typename ClassType, typename ReturnType, typename... Args>
class ReturnMethodBind final : public MethodBind
{
public:
	using Method = ReturnType (ClassType::*)(Args...);

public:
	explicit ReturnMethodBind(Method method) : m_method(method) {}

	Variant Invoke(Object* object, std::vector<Variant> const& arguments) const override
	{
		if (arguments.size() != sizeof...(Args))
		{
			throw std::invalid_argument("Incorrect method argument count");
		}

		if (object == nullptr)
		{
			throw std::invalid_argument("Object method requires a valid object");
		}

		ClassType& instance = static_cast<ClassType&>(*object);

		return InvokeMethod(instance, arguments, std::index_sequence_for<Args...>{});
	}

private:
	template <std::size_t... Indices>
	Variant
	InvokeMethod(ClassType& instance, std::vector<Variant> const& arguments, std::index_sequence<Indices...>) const
	{
		// Judge weather the return type is void
		// If the return type is void, we need to return a empty Variant
		// We need to do this in compile time, so we use if constexpr
		if constexpr (std::is_void_v<ReturnType>)
		{
			(instance.*m_method)(VariantCaster<Args>::Cast(arguments[Indices])...);
			return Variant();
		}
		else
		{
			ReturnType result = (instance.*m_method)(VariantCaster<Args>::Cast(arguments[Indices])...);
			return Variant(result);
		}
	}

private:
	Method m_method;
};

template <typename ClassType, typename ReturnType, typename... Args>
class ConstReturnMethodBind final : public MethodBind
{
public:
	using Method = ReturnType (ClassType::*)(Args...) const;

public:
	explicit ConstReturnMethodBind(Method method) : m_method(method) {}

	Variant Invoke(Object* object, std::vector<Variant> const& arguments) const override
	{
		if (arguments.size() != sizeof...(Args))
		{
			throw std::invalid_argument("Incorrect method argument count");
		}

		if (object == nullptr)
		{
			throw std::invalid_argument("Object method requires a valid object");
		}

		ClassType const& instance = static_cast<ClassType const&>(*object);

		return InvokeMethod(instance, arguments, std::index_sequence_for<Args...>{});
	}

private:
	template <std::size_t... Indices>
	Variant InvokeMethod(
		ClassType const& instance, std::vector<Variant> const& arguments, std::index_sequence<Indices...>) const
	{
		if constexpr (std::is_void_v<ReturnType>)
		{
			(instance.*m_method)(VariantCaster<Args>::Cast(arguments[Indices])...);
			return Variant();
		}
		else
		{
			ReturnType result = (instance.*m_method)(VariantCaster<Args>::Cast(arguments[Indices])...);
			return Variant(result);
		}
	}

private:
	Method m_method;
};

template <typename ReturnType, typename... Args>
class GlobalMethodBind final : public MethodBind
{
public:
	using Method = ReturnType (*)(Args...);

public:
	explicit GlobalMethodBind(Method method) : m_method(method) {}

	Variant Invoke([[maybe_unused]] Object* object, std::vector<Variant> const& arguments) const override
	{
		if (arguments.size() != sizeof...(Args))
		{
			throw std::invalid_argument("Incorrect method argument count");
		}

		return InvokeMethod(arguments, std::index_sequence_for<Args...>{});
	}

private:
	template <std::size_t... Indices>
	Variant InvokeMethod(std::vector<Variant> const& arguments, std::index_sequence<Indices...>) const
	{
		if constexpr (std::is_void_v<ReturnType>)
		{
			(*m_method)(VariantCaster<Args>::Cast(arguments[Indices])...);
			return Variant();
		}
		else
		{
			ReturnType result = (*m_method)(VariantCaster<Args>::Cast(arguments[Indices])...);
			return Variant(result);
		}
	}

private:
	Method m_method;
};
