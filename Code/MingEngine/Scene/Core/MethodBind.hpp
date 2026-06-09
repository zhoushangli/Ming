#pragma once

#include "MingEngine/Scene/Core/Object.hpp"
#include "MingEngine/Scene/Core/Variant.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

class MethodBind
{
public:
	virtual ~MethodBind() = default;

	virtual Variant Invoke(Object& object, std::vector<Variant> const& arguments) const = 0;
};

template <typename ClassType, typename... Args>
class VoidMethodBind final : public MethodBind
{
public:
	using Method = void (ClassType::*)(Args...);

public:
	explicit VoidMethodBind(Method method) : m_method(method) {}

	Variant Invoke(Object& object, std::vector<Variant> const& arguments) const override
	{
		if (arguments.size() != sizeof...(Args))
		{
			throw std::invalid_argument("Incorrect method argument count");
		}

		ClassType& instance = static_cast<ClassType&>(object);

		InvokeMethod(instance, arguments, std::index_sequence_for<Args...>{});

		return {};
	}

private:
	template <std::size_t... Indices>
	void InvokeMethod(ClassType& instance, std::vector<Variant> const& arguments, std::index_sequence<Indices...>) const
	{
		(instance.*m_method)(VariantCaster<Args>::Cast(arguments[Indices])...);
	}

private:
	Method m_method;
};

template <typename ClassType, typename ReturnType, typename... Args>
class ReturnMethodBind final : public MethodBind
{
public:
	using Method = ReturnType (ClassType::*)(Args...);

public:
	explicit ReturnMethodBind(Method method) : m_method(method) {}

	Variant Invoke(Object& object, std::vector<Variant> const& arguments) const override
	{
		if (arguments.size() != sizeof...(Args))
		{
			throw std::invalid_argument("Incorrect method argument count");
		}

		ClassType& instance = static_cast<ClassType&>(object);

		return InvokeMethod(instance, arguments, std::index_sequence_for<Args...>{});
	}

private:
	template <std::size_t... Indices>
	Variant InvokeMethod(
		ClassType& instance, std::vector<Variant> const& arguments, std::index_sequence<Indices...>) const
	{
		ReturnType result = (instance.*m_method)(VariantCaster<Args>::Cast(arguments[Indices])...);

		return Variant(result);
	}

private:
	Method m_method;
};

template <typename ClassType, typename... Args>
class ConstVoidMethodBind final : public MethodBind
{
public:
	using Method = void (ClassType::*)(Args...) const;

public:
	explicit ConstVoidMethodBind(Method method) : m_method(method) {}

	Variant Invoke(Object& object, std::vector<Variant> const& arguments) const override
	{
		if (arguments.size() != sizeof...(Args))
		{
			throw std::invalid_argument("Incorrect method argument count");
		}

		ClassType const& instance = static_cast<ClassType const&>(object);

		InvokeMethod(instance, arguments, std::index_sequence_for<Args...>{});

		return {};
	}

private:
	template <std::size_t... Indices>
	void InvokeMethod(ClassType const& instance, std::vector<Variant> const& arguments, std::index_sequence<Indices...>) const
	{
		(instance.*m_method)(VariantCaster<Args>::Cast(arguments[Indices])...);
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

	Variant Invoke(Object& object, std::vector<Variant> const& arguments) const override
	{
		if (arguments.size() != sizeof...(Args))
		{
			throw std::invalid_argument("Incorrect method argument count");
		}

		ClassType const& instance = static_cast<ClassType const&>(object);

		return InvokeMethod(instance, arguments, std::index_sequence_for<Args...>{});
	}

private:
	template <std::size_t... Indices>
	Variant InvokeMethod(
		ClassType const& instance, std::vector<Variant> const& arguments, std::index_sequence<Indices...>) const
	{
		ReturnType result = (instance.*m_method)(VariantCaster<Args>::Cast(arguments[Indices])...);

		return Variant(result);
	}

private:
	Method m_method;
};