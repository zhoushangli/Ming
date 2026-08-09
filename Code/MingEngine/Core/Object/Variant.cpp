#include "MingEngine/Core/Object/Variant.hpp"

Variant::Variant(bool value) : m_value(value), m_type(Type::Bool) {}
Variant::Variant(int value) : m_value(value), m_type(Type::Int) {}
Variant::Variant(float value) : m_value(value), m_type(Type::Float) {}
Variant::Variant(char const* value) : m_value(std::string(value)), m_type(Type::String) {}
Variant::Variant(std::string const& value) : m_value(value), m_type(Type::String) {}
Variant::Variant(Vector2 const& value) : m_value(value), m_type(Type::Vector2) {}
Variant::Variant(Vector3 const& value) : m_value(value), m_type(Type::Vector3) {}
Variant::Variant(Vector4 const& value) : m_value(value), m_type(Type::Vector4) {}
Variant::Variant(Color const& value) : m_value(value), m_type(Type::Color) {}
Variant::Variant(AABB2 const& value) : m_value(value), m_type(Type::AABB2) {}
Variant::Variant(OBB2 const& value) : m_value(value), m_type(Type::OBB2) {}
Variant::Variant(Capsule3 const& value) : m_value(value), m_type(Type::Capsule3) {}
Variant::Variant(EulerAngles const& value) : m_value(value), m_type(Type::EulerAngles) {}
Variant::Variant(Matrix4x4 const& value) : m_value(value), m_type(Type::Matrix4x4) {}
Variant::Variant(Object* const& value) : m_value(value), m_type(Type::ObjectPtr) {}

bool Variant::IsEmpty() const { return std::holds_alternative<std::monostate>(m_value); }

Variant::Type Variant::GetType() const { return m_type; }

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
		return As<std::string>() == other.As<std::string>();
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
