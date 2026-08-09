#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/Capsule3.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/OBB2.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"
#include "MingEngine/Core/Math/Vector4.hpp"
#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"

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
		Vector2,
		Vector3,
		Vector4,
		Color,
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
		Vector2,
		Vector3,
		Vector4,
		Color,
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
	Variant(Vector2 const& value);
	Variant(Vector3 const& value);
	Variant(Vector4 const& value);
	Variant(Color const& value);
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
		if (std::is_same_v<CleanType, Vector2>)
			return Type::Vector2;
		if (std::is_same_v<CleanType, Vector3>)
			return Type::Vector3;
		if (std::is_same_v<CleanType, Vector4>)
			return Type::Vector4;
		if (std::is_same_v<CleanType, Color>)
			return Type::Color;
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
		if (std::is_same_v<CleanType, Variant>)
			return Type::Any;
		if constexpr (std::is_base_of_v<Object, std::remove_pointer_t<CleanType>>)
			return Type::ObjectPtr;
	}

private:
	Type    m_type = Type::Empty;
	Storage m_value;
};

template <typename T>
struct VariantCaster
{
	static T Cast(Variant const& value)
	{
		using CleanType   = std::remove_cv_t<std::remove_reference_t<T>>;
		using PointeeType = std::remove_pointer_t<CleanType>;
		using RefType     = std::remove_reference_t<T>;

		// Raw Variant.
		// Example: void Foo(Variant const& value)
		if constexpr (std::is_same_v<CleanType, Variant>)
		{
			return value;
		}
		// Object pointer.
		// Example: void AddNode(Node* child)
		else if constexpr (std::is_pointer_v<CleanType> && std::is_base_of_v<Object, PointeeType>)
		{
			return dynamic_cast<CleanType>(value.As<Object*>());
		}
		else
		{
			// Mutable reference.
			// Example: bool PushOut(Vec2& position)
			if constexpr (std::is_lvalue_reference_v<T> && !std::is_const_v<RefType>)
			{
				return const_cast<CleanType&>(value.As<CleanType>());
			}
			else
			{
				// Value or const reference.
				// Example: void SetPosition(Vec3 const& position)
				return value.As<CleanType>();
			}
		}
	}
};
