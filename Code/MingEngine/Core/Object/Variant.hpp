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
#include "MingEngine/Core/Render/Color.hpp"
#include "MingEngine/Core/String.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

// Hold one value of any builtin type inside 24 bytes: a 4 byte tag, 4 bytes of padding and a
// 16 byte payload, which is the shape the Godot Variant uses.
// The payload keeps the small types inline, holds the String in place and keeps the types that
// are wider than the payload behind an owned pointer.
// e.g. Variant(Vector3::One) stores 12 bytes inline, Variant(Matrix4x4()) owns a Matrix4x4.
class Variant
{
public:
	enum class Type : int32_t
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

	// Element count of the payload, every inline type has to fit into that many bytes.
	static constexpr uint32_t kPayloadSize = 16;

private:
	// Only scalars and pointers are named here: a union member with a user provided constructor
	// or destructor would delete the special members of the union itself.
	union Payload
	{
		bool    m_bool;
		int32_t m_int;
		float   m_float;
		void*   m_pointer;
		uint8_t m_bytes[kPayloadSize];
	};

public:
	Variant() = default;

	// 1) Copy or steal the payload of the other variant
	// 2) Keep the tag, so a copied String shares its text and an owned type is copied by value
	// e.g. Variant copy = Variant(Matrix4x4()) allocates a second Matrix4x4 for the copy.
	Variant(Variant const& other);
	Variant(Variant&& other) noexcept;
	Variant& operator=(Variant const& other);
	Variant& operator=(Variant&& other) noexcept;
	~Variant();

	Variant(bool value);
	Variant(int value);
	Variant(float value);
	Variant(char const* value);

	// UTF-8 text is decoded into the owned String value.
	// e.g. Variant(std::string("res://A.png")) keeps the same text as Variant("res://A.png").
	Variant(std::string const& value);
	Variant(String const& value);
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
		return m_type == GetType<T>();
	}

	// Read the stored value. Types that do not fit the payload are owned behind a pointer, so
	// they are reached through that pointer.
	// e.g. Variant(Vector3::One).As<Vector3>() returns the inline Vector3.
	template <typename T>
	T& As()
	{
		if constexpr (IsOwned<T>())
		{
			return *static_cast<T*>(m_payload.m_pointer);
		}
		else
		{
			return *reinterpret_cast<T*>(m_payload.m_bytes);
		}
	}

	template <typename T>
	T const& As() const
	{
		return const_cast<Variant*>(this)->As<T>();
	}

	// Report the tag that represents T. A type missing from this list fails to compile on purpose:
	// a silent wrong tag would be written into every value of that type and the binding generator
	// would then skip the methods that use it.
	template <typename T>
	static Type GetType()
	{
		using CleanType = std::remove_cv_t<std::remove_reference_t<T>>;

		if constexpr (std::is_same_v<CleanType, void>)
			return Type::Empty;
		else if constexpr (std::is_same_v<CleanType, bool>)
			return Type::Bool;
		else if constexpr (std::is_same_v<CleanType, int>)
			return Type::Int;
		else if constexpr (std::is_same_v<CleanType, float>)
			return Type::Float;
		else if constexpr (std::is_same_v<CleanType, String>)
			return Type::String;
		else if constexpr (std::is_same_v<CleanType, Vector2>)
			return Type::Vector2;
		else if constexpr (std::is_same_v<CleanType, Vector3>)
			return Type::Vector3;
		else if constexpr (std::is_same_v<CleanType, Vector4>)
			return Type::Vector4;
		else if constexpr (std::is_same_v<CleanType, Color>)
			return Type::Color;
		else if constexpr (std::is_same_v<CleanType, AABB2>)
			return Type::AABB2;
		else if constexpr (std::is_same_v<CleanType, OBB2>)
			return Type::OBB2;
		else if constexpr (std::is_same_v<CleanType, Capsule3>)
			return Type::Capsule3;
		else if constexpr (std::is_same_v<CleanType, EulerAngles>)
			return Type::EulerAngles;
		else if constexpr (std::is_same_v<CleanType, Matrix4x4>)
			return Type::Matrix4x4;
		else if constexpr (std::is_same_v<CleanType, Variant>)
			return Type::Any;
		else if constexpr (std::is_base_of_v<Object, std::remove_pointer_t<CleanType>>)
			return Type::ObjectPtr;
		else
		{
			static_assert(sizeof(CleanType) == 0, "Missing Variant tag for this type.");
		}
	}

private:
	// Types wider than the payload are stored behind an owned pointer instead.
	template <typename T>
	static constexpr bool IsOwned()
	{
		using CleanType = std::remove_cv_t<std::remove_reference_t<T>>;

		return std::is_same_v<CleanType, OBB2> || std::is_same_v<CleanType, Capsule3>
			   || std::is_same_v<CleanType, Matrix4x4>;
	}

	// 1) Copy the bytes of the value into the payload
	// 2) Leave the tag to the caller, so the tag always describes the stored bytes
	// e.g. Write(Vector3::One) stores 12 bytes and ignores the remaining 4.
	template <typename T>
	void Write(T const& value)
	{
		static_assert(sizeof(T) <= kPayloadSize, "This type needs the owned pointer, see IsOwned().");

		std::memcpy(m_payload.m_bytes, &value, sizeof(T));
	}

	void CopyFrom(Variant const& other);
	void MoveFrom(Variant& other);

	// 1) Release whatever the payload owns right now
	// 2) Leave the variant empty so the same storage can be reused
	// e.g. Clear() on a Variant holding a Matrix4x4 deletes that Matrix4x4.
	void Clear();

private:
	Type    m_type    = Type::Empty;
	Payload m_payload = {};
};

static_assert(sizeof(Variant) == 24, "Variant must stay 24 bytes so the tag and the payload fit.");

template <typename T>
struct VariantCaster
{
	static T Cast(Variant const& value)
	{
		using CleanType   = std::remove_cv_t<std::remove_reference_t<T>>;
		using PointeeType = std::remove_cv_t<std::remove_pointer_t<CleanType>>;
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
