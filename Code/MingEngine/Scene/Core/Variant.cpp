#include "MingEngine/Scene/Core/Variant.hpp"

Variant::Variant(bool value) : m_value(value) {}

Variant::Variant(int value) : m_value(value) {}

Variant::Variant(float value) : m_value(value) {}

Variant::Variant(char const* value) : m_value(std::string(value)) {}

Variant::Variant(std::string const& value) : m_value(value) {}

Variant::Variant(Vec3 const& value) : m_value(value) {}

Variant::Variant(EulerAngles const& value) : m_value(value) {}

Variant::Variant(Matrix4x4 const& value) : m_value(value) {}

bool Variant::IsEmpty() const { return std::holds_alternative<std::monostate>(m_value); }