#include "MingEngine/Core/Object/Variant.hpp"

#include "MingEngine/Core/Memory.hpp"

#include <new>
#include <utility>

// 1) Copy or steal the payload of the other variant
// 2) Keep the tag, so a copy always describes the bytes it holds
// e.g. Copying a Variant holding a String adds one owner to the same text block.
Variant::Variant(Variant const& other) { CopyFrom(other); }

Variant::Variant(Variant&& other) noexcept { MoveFrom(other); }

Variant& Variant::operator=(Variant const& other)
{
	if (this != &other)
	{
		// 1) Release the payload this variant owns
		// 2) Copy the other payload, which may share the block that was just released
		Clear();
		CopyFrom(other);
	}

	return *this;
}

Variant& Variant::operator=(Variant&& other) noexcept
{
	if (this != &other)
	{
		Clear();
		MoveFrom(other);
	}

	return *this;
}

Variant::~Variant() { Clear(); }

Variant::Variant(bool value) : m_type(Type::Bool) { m_payload.m_bool = value; }

Variant::Variant(int value) : m_type(Type::Int) { m_payload.m_int = value; }

Variant::Variant(float value) : m_type(Type::Float) { m_payload.m_float = value; }

// Text is stored in place, so the payload owns a String that shares its code points.
// e.g. Variant("res://A.png") keeps the decoded text alive for as long as this variant lives.
Variant::Variant(char const* value) : m_type(Type::String) { new (m_payload.m_bytes) String(value); }

Variant::Variant(std::string const& value) : m_type(Type::String) { new (m_payload.m_bytes) String(value); }

Variant::Variant(String const& value) : m_type(Type::String) { new (m_payload.m_bytes) String(value); }

Variant::Variant(Vector2 const& value) : m_type(Type::Vector2) { Write(value); }

Variant::Variant(Vector3 const& value) : m_type(Type::Vector3) { Write(value); }

Variant::Variant(Vector4 const& value) : m_type(Type::Vector4) { Write(value); }

Variant::Variant(Color const& value) : m_type(Type::Color) { Write(value); }

Variant::Variant(AABB2 const& value) : m_type(Type::AABB2) { Write(value); }

Variant::Variant(EulerAngles const& value) : m_type(Type::EulerAngles) { Write(value); }

// These types are wider than the payload, so this variant owns a heap copy of the value.
// e.g. Variant(Matrix4x4()) owns one Matrix4x4 that ~Variant deletes.
Variant::Variant(OBB2 const& value) : m_type(Type::OBB2) { m_payload.m_pointer = MemNew<OBB2>(value); }

Variant::Variant(Capsule3 const& value) : m_type(Type::Capsule3) { m_payload.m_pointer = MemNew<Capsule3>(value); }

Variant::Variant(Matrix4x4 const& value) : m_type(Type::Matrix4x4) { m_payload.m_pointer = MemNew<Matrix4x4>(value); }

Variant::Variant(Object* const& value) : m_type(Type::ObjectPtr) { m_payload.m_pointer = value; }

bool Variant::IsEmpty() const { return m_type == Type::Empty; }

Variant::Type Variant::GetType() const { return m_type; }

// 1) Copy the value the other variant holds
// 2) Adopt the tag of the other variant
// e.g. CopyFrom() adds one owner to a String and copies an AABB2 as 16 bytes.
void Variant::CopyFrom(Variant const& other)
{
	m_type = other.m_type;

	switch (m_type)
	{
	case Type::String:
		new (m_payload.m_bytes) String(other.As<String>());
		break;
	case Type::OBB2:
		m_payload.m_pointer = MemNew<OBB2>(other.As<OBB2>());
		break;
	case Type::Capsule3:
		m_payload.m_pointer = MemNew<Capsule3>(other.As<Capsule3>());
		break;
	case Type::Matrix4x4:
		m_payload.m_pointer = MemNew<Matrix4x4>(other.As<Matrix4x4>());
		break;
	default:
		m_payload = other.m_payload;
		break;
	}
}

// 1) Take over the payload of the other variant
// 2) Leave the other variant empty without releasing what was taken over
// e.g. MoveFrom() transfers a Matrix4x4 pointer instead of copying the matrix.
void Variant::MoveFrom(Variant& other)
{
	m_type = other.m_type;

	switch (m_type)
	{
	case Type::String:
		new (m_payload.m_bytes) String(std::move(other.As<String>()));
		other.As<String>().~String();
		break;
	case Type::OBB2:
	case Type::Capsule3:
	case Type::Matrix4x4:
		m_payload.m_pointer = other.m_payload.m_pointer;
		break;
	default:
		m_payload = other.m_payload;
		break;
	}

	other.m_type = Type::Empty;
}

// 1) Release whatever the payload owns right now
// 2) Leave the variant empty so the same storage can be reused
// e.g. Clear() on a Variant holding a Matrix4x4 deletes that Matrix4x4.
void Variant::Clear()
{
	switch (m_type)
	{
	case Type::String:
		reinterpret_cast<String*>(m_payload.m_bytes)->~String();
		break;
	case Type::OBB2:
		MemDelete(static_cast<OBB2*>(m_payload.m_pointer));
		break;
	case Type::Capsule3:
		MemDelete(static_cast<Capsule3*>(m_payload.m_pointer));
		break;
	case Type::Matrix4x4:
		MemDelete(static_cast<Matrix4x4*>(m_payload.m_pointer));
		break;
	default:
		break;
	}

	m_type = Type::Empty;
}

bool Variant::operator==(Variant const& other) const
{
	if (m_type != other.m_type)
	{
		return false;
	}

	switch (m_type)
	{
	case Type::Empty:
		return true;
	case Type::Bool:
		return As<bool>() == other.As<bool>();
	case Type::Int:
		return As<int>() == other.As<int>();
	case Type::Float:
		return As<float>() == other.As<float>();
	case Type::String:
		return As<String>() == other.As<String>();
	case Type::Vector2:
		return As<Vector2>() == other.As<Vector2>();
	case Type::Vector3:
		return As<Vector3>() == other.As<Vector3>();
	case Type::Vector4:
		return As<Vector4>() == other.As<Vector4>();
	case Type::Color:
		return As<Color>() == other.As<Color>();
	case Type::AABB2:
		return As<AABB2>() == other.As<AABB2>();
	case Type::OBB2:
	{
		OBB2 const& left  = As<OBB2>();
		OBB2 const& right = other.As<OBB2>();
		return left.m_center == right.m_center && left.m_iBasisNormal == right.m_iBasisNormal
			   && left.m_halfDimensions == right.m_halfDimensions;
	}
	case Type::Capsule3:
	{
		Capsule3 const& left  = As<Capsule3>();
		Capsule3 const& right = other.As<Capsule3>();
		return left.m_start == right.m_start && left.m_end == right.m_end && left.m_radius == right.m_radius;
	}
	case Type::EulerAngles:
	{
		EulerAngles const& left  = As<EulerAngles>();
		EulerAngles const& right = other.As<EulerAngles>();
		return left.m_yawDegrees == right.m_yawDegrees && left.m_pitchDegrees == right.m_pitchDegrees
			   && left.m_rollDegrees == right.m_rollDegrees;
	}
	case Type::Matrix4x4:
	{
		float const* left  = As<Matrix4x4>().GetAsFloatArray();
		float const* right = other.As<Matrix4x4>().GetAsFloatArray();
		for (int index = 0; index < 16; ++index)
		{
			if (left[index] != right[index])
			{
				return false;
			}
		}
		return true;
	}
	case Type::ObjectPtr:
	{
		return As<Object*>() == other.As<Object*>();
	}
	}

	return false;
}

bool Variant::operator!=(Variant const& other) const { return !(*this == other); }
