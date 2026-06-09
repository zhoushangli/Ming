#pragma once

#include "MingEngine/Engine/Math/EulerAngles.hpp"
#include "MingEngine/Engine/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Math/Vec3.hpp"

#include <string>
#include <utility>
#include <variant>

class Variant
{
public:
	// The type of the underlying storage for the variant.
	using Storage = std::variant<std::monostate, bool, int, float, std::string, Vec3, EulerAngles, Matrix4x4>;

public:
	Variant() = default;

	template <typename T>
	Variant(T&& value) : m_value(std::forward<T>(value))
	{
	}

	bool IsEmpty() const;

	template <typename T>
	bool Is() const
	{
		return std::holds_alternative<T>(m_value);
	}

	template <typename T>
	T& As()
	{
		return std::get<T>(m_value);
	}

	template <typename T>
	T const& As()
	{
		return std::get<T>(m_value);
	}

	Storage const& GetStorage() const;

private:
	Storage m_value;
};
