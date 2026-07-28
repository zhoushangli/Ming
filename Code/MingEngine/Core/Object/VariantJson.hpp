#pragma once

#include "MingEngine/Core/Object/Variant.hpp"

#include "ThirdParty/nlohmann/json.hpp"

namespace VariantJson
{
using Json = nlohmann::ordered_json;

// This function serializes a Variant into a JSON value.
// e.g. Variant(Vec3(1, 2, 3)) becomes [1, 2, 3].
bool TrySerialize(Variant const& value, Json& outJson);

// This function deserializes a JSON value into a Variant with the expected type.
// e.g. [1, 2, 3] with Variant::Type::Vec3 becomes Variant(Vec3(1, 2, 3)).
bool TryDeserialize(Json const& json, Variant::Type expectedType, Variant& outValue);
} // namespace VariantJson
