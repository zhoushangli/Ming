#pragma once

#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

template <typename T>
struct PtrToArg
{
	using ReferenceType = std::remove_reference_t<T>;
	using ValueType     = std::remove_cv_t<ReferenceType>;

	// The Decode here reference to we want to get the actual value
	// form an unknown pointer
	static decltype(auto) Decode(void* ptr)
	{
		if constexpr (std::is_lvalue_reference_v<T> && !std::is_const_v<ReferenceType>)
		{
			return *static_cast<ValueType*>(ptr);
		}
		else
		{
			return *static_cast<ValueType const*>(ptr);
		}
	}

	// The Encode here reference to we want to set the actual value
	// to an unknown pointer
	static void Encode(ValueType const& value, void* ptr) { *static_cast<ValueType*>(ptr) = value; }
};

// bool is a special case, because for different platform, the size of bool is different
// so we need to explicitly cast this to uint8_t, which is one byte, and this aligned with C# side code
template <>
struct PtrToArg<bool>
{
	static bool Decode(void* ptr) { return *static_cast<uint8_t const*>(ptr) != 0; }

	static void Encode(bool value, void* ptr) { *static_cast<uint8_t*>(ptr) = value ? 1 : 0; }
};

struct MingString
{
	std::string* m_string;
};

template <>
struct PtrToArg<std::string>
{
	static std::string const& Decode(void* ptr)
	{
		MingString const* mingString = static_cast<MingString const*>(ptr);

		return *mingString->m_string;
	}

	static void Encode(std::string const& value, void* ptr)
	{
		MingString* mingString = static_cast<MingString*>(ptr);

		mingString->m_string = new std::string(value);
	}
};

template <>
struct PtrToArg<std::string const&> : PtrToArg<std::string>
{
};

class MethodBind
{
public:
	virtual ~MethodBind() = default;

	virtual Variant Invoke(Object* object, std::vector<Variant> const& arguments) const = 0;

	virtual void PtrCall(Object* object, void** args, void* retPtr) const = 0;
};

template <typename ClassType, typename ReturnType, typename... Args>
class MemberMethodBind final : public MethodBind
{
public:
	using Method = ReturnType (ClassType::*)(Args...);

public:
	explicit MemberMethodBind(Method method) : m_method(method) {}

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

	void PtrCall(Object* object, void** args, void* retPtr) const override
	{
		ClassType& instance = static_cast<ClassType&>(*object);
		PtrCallMethod(instance, args, retPtr, std::index_sequence_for<Args...>{});
	}

private:
	template <std::size_t... Indices>
	Variant InvokeMethod(
		ClassType& instance, std::vector<Variant> const& arguments, std::index_sequence<Indices...>) const
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

	template <std::size_t... Indices>
	void PtrCallMethod(ClassType& instance, void** args, void* retPtr, std::index_sequence<Indices...>) const
	{
		if constexpr (std::is_void_v<ReturnType>)
		{
			(instance.*m_method)(PtrToArg<Args>::Decode(args[Indices])...);
		}
		else
		{
			ReturnType result = (instance.*m_method)(PtrToArg<Args>::Decode(args[Indices])...);
			PtrToArg<ReturnType>::Encode(result, retPtr);
		}
	}

private:
	Method m_method;
};

template <typename ClassType, typename ReturnType, typename... Args>
class ConstMemberMethodBind final : public MethodBind
{
public:
	using Method = ReturnType (ClassType::*)(Args...) const;

public:
	explicit ConstMemberMethodBind(Method method) : m_method(method) {}

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

	void PtrCall(Object* object, void** args, void* retPtr) const override
	{
		ClassType const& instance = static_cast<ClassType const&>(*object);
		PtrCallMethod(instance, args, retPtr, std::index_sequence_for<Args...>{});
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

	template <std::size_t... Indices>
	void PtrCallMethod(ClassType const& instance, void** args, void* retPtr, std::index_sequence<Indices...>) const
	{
		if constexpr (std::is_void_v<ReturnType>)
		{
			(instance.*m_method)(PtrToArg<Args>::Decode(args[Indices])...);
		}
		else
		{
			ReturnType result = (instance.*m_method)(PtrToArg<Args>::Decode(args[Indices])...);
			PtrToArg<ReturnType>::Encode(result, retPtr);
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

	void PtrCall([[maybe_unused]] Object* object, void** args, void* retPtr) const override
	{
		PtrCallMethod(args, retPtr, std::index_sequence_for<Args...>{});
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

	template <std::size_t... Indices>
	void PtrCallMethod(void** args, void* retPtr, std::index_sequence<Indices...>) const
	{
		if constexpr (std::is_void_v<ReturnType>)
		{
			(*m_method)(PtrToArg<Args>::Decode(args[Indices])...);
		}
		else
		{
			ReturnType result = (*m_method)(PtrToArg<Args>::Decode(args[Indices])...);
			PtrToArg<ReturnType>::Encode(result, retPtr);
		}
	}

private:
	Method m_method;
};
