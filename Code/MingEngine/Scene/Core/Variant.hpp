#pragma once

#include "MingEngine/Engine/Math/EulerAngles.hpp"
#include "MingEngine/Engine/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Math/Vec3.hpp"

#include <string>
#include <variant>

class Variant
{
public:
	using Storage = std::variant<
		std::monostate,
		bool,
		int,
		float,
		std::string,
		Vec3,
		EulerAngles,
		Matrix4x4>;

public:
	Variant() = default;

	Variant(bool value);
	Variant(int value);
	Variant(float value);
	Variant(char const* value);
	Variant(std::string const& value);
	Variant(Vec3 const& value);
	Variant(EulerAngles const& value);
	Variant(Matrix4x4 const& value);

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
	T const& As() const
	{
		return std::get<T>(m_value);
	}

private:
	Storage m_value;
};

template <typename T>
struct VariantCaster;

template <>
struct VariantCaster<bool>
{
	static bool Cast(Variant const& value)
	{
		return value.As<bool>();
	}
};

template <>
struct VariantCaster<int>
{
	static int Cast(Variant const& value)
	{
		return value.As<int>();
	}
};

template <>
struct VariantCaster<float>
{
	static float Cast(Variant const& value)
	{
		return value.As<float>();
	}
};

template <>
struct VariantCaster<Vec3>
{
	static Vec3 Cast(Variant const& value)
	{
		return value.As<Vec3>();
	}
};

template <>
struct VariantCaster<Vec3 const&>
{
	static Vec3 const& Cast(Variant const& value)
	{
		return value.As<Vec3>();
	}
};

template <>
struct VariantCaster<std::string>
{
	static std::string Cast(Variant const& value)
	{
		return value.As<std::string>();
	}
};

template <>
struct VariantCaster<std::string const&>
{
	static std::string const& Cast(Variant const& value)
	{
		return value.As<std::string>();
	}
};