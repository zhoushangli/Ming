#include "MingEngine/Scene/Core/Variant.hpp"

bool Variant::IsEmpty() const { return std::holds_alternative<std::monostate>(m_value); }

Variant::Storage const& Variant::GetStorage() const { return m_value; }
