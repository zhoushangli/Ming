#include "MingEngine/Core/Object/Variant.hpp"

Variant::Variant(bool value) : m_value(value), m_type(Type::Bool) {}
Variant::Variant(int value) : m_value(value), m_type(Type::Int) {}
Variant::Variant(float value) : m_value(value), m_type(Type::Float) {}
Variant::Variant(char const* value) : m_value(std::string(value)), m_type(Type::String) {}
Variant::Variant(std::string const& value) : m_value(value), m_type(Type::String) {}
Variant::Variant(Vec3 const& value) : m_value(value), m_type(Type::Vec3) {}
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
	case Type::Vec3:
		return As<Vec3>() == other.As<Vec3>();
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
	}

	return false;
}

bool Variant::operator!=(Variant const& other) const { return !(*this == other); }
