#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/Capsule3.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/OBB2.hpp"
#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Core/Math/Vec4.hpp"
#include "MingEngine/Core/Object/Object.hpp"

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
		Vec2,
		Vec3,
		Vec4,
		AABB2,
		OBB2,
		Capsule3,
		EulerAngles,
		Matrix4x4,
		Object*>;

	enum class Type
	{
		Empty,
		Bool,
		Int,
		Float,
		String,
		Vec2,
		Vec3,
		Vec4,
		AABB2,
		OBB2,
		Capsule3,
		EulerAngles,
		Matrix4x4,
		ObjectPtr,
		Any, // This is a special type used for method binding, it means the method can accept any type of argument
	};

public:
	Variant() = default;

	Variant(bool value);
	Variant(int value);
	Variant(float value);
	Variant(char const* value);
	Variant(std::string const& value);
	Variant(Vec2 const& value);
	Variant(Vec3 const& value);
	Variant(Vec4 const& value);
	Variant(AABB2 const& value);
	Variant(OBB2 const& value);
	Variant(Capsule3 const& value);
	Variant(EulerAngles const& value);
	Variant(Matrix4x4 const& value);
	Variant(Object* const& value);
	bool IsEmpty() const;
	Type GetType() const;
	bool operator==(Variant const& other) const;
	bool operator!=(Variant const& other) const;

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

	template <typename T>
	static Type GetType()
	{
		using CleanType = std::remove_cv_t<std::remove_reference_t<T>>;

		if (std::is_same_v<CleanType, void>)
			return Type::Empty;
		if (std::is_same_v<CleanType, bool>)
			return Type::Bool;
		if (std::is_same_v<CleanType, int>)
			return Type::Int;
		if (std::is_same_v<CleanType, float>)
			return Type::Float;
		if (std::is_same_v<CleanType, std::string>)
			return Type::String;
		if (std::is_same_v<CleanType, Vec2>)
			return Type::Vec2;
		if (std::is_same_v<CleanType, Vec3>)
			return Type::Vec3;
		if (std::is_same_v<CleanType, Vec4>)
			return Type::Vec4;
		if (std::is_same_v<CleanType, AABB2>)
			return Type::AABB2;
		if (std::is_same_v<CleanType, OBB2>)
			return Type::OBB2;
		if (std::is_same_v<CleanType, Capsule3>)
			return Type::Capsule3;
		if (std::is_same_v<CleanType, EulerAngles>)
			return Type::EulerAngles;
		if (std::is_same_v<CleanType, Matrix4x4>)
			return Type::Matrix4x4;
		if (std::is_same_v<CleanType, Object*>)
			return Type::ObjectPtr;
		if (std::is_same_v<CleanType, Variant>)
			return Type::Any;
	}

private:
	Type    m_type = Type::Empty;
	Storage m_value;
};

template <typename T>
struct VariantCaster;

template <>
struct VariantCaster<bool>
{
	static bool Cast(Variant const& value) { return value.As<bool>(); }
};

template <>
struct VariantCaster<int>
{
	static int Cast(Variant const& value) { return value.As<int>(); }
};

template <>
struct VariantCaster<float>
{
	static float Cast(Variant const& value) { return value.As<float>(); }
};

template <>
struct VariantCaster<Vec2>
{
	static Vec2 Cast(Variant const& value) { return value.As<Vec2>(); }
};

template <>
struct VariantCaster<Vec2&>
{
	static Vec2& Cast(Variant const& value) { return const_cast<Vec2&>(value.As<Vec2>()); }
};

template <>
struct VariantCaster<Vec2 const&>
{
	static Vec2 const& Cast(Variant const& value) { return value.As<Vec2>(); }
};

template <>
struct VariantCaster<Vec3>
{
	static Vec3 Cast(Variant const& value) { return value.As<Vec3>(); }
};

template <>
struct VariantCaster<Vec3&>
{
	static Vec3& Cast(Variant const& value) { return const_cast<Vec3&>(value.As<Vec3>()); }
};

template <>
struct VariantCaster<Vec3 const&>
{
	static Vec3 const& Cast(Variant const& value) { return value.As<Vec3>(); }
};

template <>
struct VariantCaster<Vec4>
{
	static Vec4 Cast(Variant const& value) { return value.As<Vec4>(); }
};

template <>
struct VariantCaster<Vec4&>
{
	static Vec4& Cast(Variant const& value) { return const_cast<Vec4&>(value.As<Vec4>()); }
};

template <>
struct VariantCaster<Vec4 const&>
{
	static Vec4 const& Cast(Variant const& value) { return value.As<Vec4>(); }
};

template <>
struct VariantCaster<AABB2>
{
	static AABB2 Cast(Variant const& value) { return value.As<AABB2>(); }
};

template <>
struct VariantCaster<AABB2&>
{
	static AABB2& Cast(Variant const& value) { return const_cast<AABB2&>(value.As<AABB2>()); }
};

template <>
struct VariantCaster<AABB2 const&>
{
	static AABB2 const& Cast(Variant const& value) { return value.As<AABB2>(); }
};

template <>
struct VariantCaster<OBB2>
{
	static OBB2 Cast(Variant const& value) { return value.As<OBB2>(); }
};

template <>
struct VariantCaster<OBB2&>
{
	static OBB2& Cast(Variant const& value) { return const_cast<OBB2&>(value.As<OBB2>()); }
};

template <>
struct VariantCaster<OBB2 const&>
{
	static OBB2 const& Cast(Variant const& value) { return value.As<OBB2>(); }
};

template <>
struct VariantCaster<Capsule3>
{
	static Capsule3 Cast(Variant const& value) { return value.As<Capsule3>(); }
};

template <>
struct VariantCaster<Capsule3&>
{
	static Capsule3& Cast(Variant const& value) { return const_cast<Capsule3&>(value.As<Capsule3>()); }
};

template <>
struct VariantCaster<Capsule3 const&>
{
	static Capsule3 const& Cast(Variant const& value) { return value.As<Capsule3>(); }
};

template <>
struct VariantCaster<EulerAngles>
{
	static EulerAngles Cast(Variant const& value) { return value.As<EulerAngles>(); }
};

template <>
struct VariantCaster<EulerAngles const&>
{
	static EulerAngles const& Cast(Variant const& value) { return value.As<EulerAngles>(); }
};

template <>
struct VariantCaster<Matrix4x4>
{
	static Matrix4x4 Cast(Variant const& value) { return value.As<Matrix4x4>(); }
};

template <>
struct VariantCaster<Matrix4x4 const&>
{
	static Matrix4x4 const& Cast(Variant const& value) { return value.As<Matrix4x4>(); }
};

template <>
struct VariantCaster<std::string>
{
	static std::string Cast(Variant const& value) { return value.As<std::string>(); }
};

template <>
struct VariantCaster<std::string const&>
{
	static std::string const& Cast(Variant const& value) { return value.As<std::string>(); }
};

template <>
struct VariantCaster<Variant>
{
	static Variant Cast(Variant const& value) { return value; }
};

template <>
struct VariantCaster<Variant const&>
{
	static Variant const& Cast(Variant const& value) { return value; }
};
