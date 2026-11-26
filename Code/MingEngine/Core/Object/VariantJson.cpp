#include "MingEngine/Core/Object/VariantJson.hpp"
#include <cmath>
#include "MingEngine/Core/StringUtils.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/IO/Resource.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"

#include <exception>
#include <limits>

namespace
{
std::string GetSerializableResourcePath(Resource const& resource)
{
	if (resource.GetSourceFilePath().IsValid())
	{
		return resource.GetSourceFilePath().GetString();
	}

	return resource.GetVirtualPath().GetString();
}

bool IsNumberArray(VariantJson::Json const& json, size_t expectedSize)
{
	if (!json.is_array() || json.size() != expectedSize)
	{
		return false;
	}

	for (VariantJson::Json const& entry : json)
	{
		if (!entry.is_number() || !std::isfinite(entry.get<float>()))
		{
			return false;
		}
	}

	return true;
}
} // namespace

namespace VariantJson
{
bool TrySerialize(Variant const& value, Json& outJson)
{
	switch (value.GetType())
	{
	case Variant::Type::Bool:
		outJson = value.As<bool>();
		return true;
	case Variant::Type::Int64:
		outJson = value.As<int64_t>();
		return true;
	case Variant::Type::UInt32:
		outJson = value.As<uint32_t>();
		return true;
	case Variant::Type::Int:
		outJson = value.As<int>();
		return true;
	case Variant::Type::Float:
		outJson = value.As<float>();
		return true;
	case Variant::Type::String:
		outJson = value.As<String>().ToUtf8();
		return true;
	case Variant::Type::Vector4:
	{
		auto const& vector = value.As<Vector4>();
		outJson = Json::array({ vector.x, vector.y, vector.z, vector.w });
		return true;
	}
	case Variant::Type::AABB2:
	{
		auto const& box = value.As<AABB2>();
		outJson = Json::array({ box.m_mins.x, box.m_mins.y, box.m_maxs.x, box.m_maxs.y });
		return true;
	}
	case Variant::Type::OBB2:
	{
		auto const& box = value.As<OBB2>();
		outJson = Json::array({ box.m_center.x, box.m_center.y, box.m_iBasisNormal.x, box.m_iBasisNormal.y, box.m_halfDimensions.x, box.m_halfDimensions.y });
		return true;
	}
	case Variant::Type::Capsule3:
	{
		auto const& capsule = value.As<Capsule3>();
		outJson = Json::array({ capsule.m_start.x, capsule.m_start.y, capsule.m_start.z, capsule.m_end.x, capsule.m_end.y, capsule.m_end.z, capsule.m_radius });
		return true;
	}
	case Variant::Type::Vector2:
	{
		Vector2 const& vector = value.As<Vector2>();
		outJson               = Json::array({ vector.x, vector.y });
		return true;
	}

	case Variant::Type::Vector3:
	{
		Vector3 const& vector = value.As<Vector3>();
		outJson               = Json::array({ vector.x, vector.y, vector.z });
		return true;
	}

	case Variant::Type::EulerAngles:
	{
		EulerAngles const& angles = value.As<EulerAngles>();
		outJson                   = Json::array({ angles.m_yawDegrees, angles.m_pitchDegrees, angles.m_rollDegrees });
		return true;
	}

	case Variant::Type::Matrix4x4:
	{
		outJson             = Json::array();
		float const* matrix = value.As<Matrix4x4>().GetAsFloatArray();
		for (int index = 0; index < 16; ++index)
		{
			outJson.push_back(matrix[index]);
		}

		return true;
	}

	case Variant::Type::Color:
	{
		Color const& color = value.As<Color>();
		outJson            = Json::array({ color.r, color.g, color.b, color.a });
		return true;
	}

	case Variant::Type::ObjectPtr:
	{
		Object* object = value.As<Object*>();
		if (object == nullptr)
		{
			outJson = nullptr;
			return true;
		}

		Resource const* resource = dynamic_cast<Resource const*>(object);
		if (resource != nullptr)
		{
			std::string const resourcePath = GetSerializableResourcePath(*resource);
			if (resourcePath.empty())
			{
				WARN_PRINT(Stringf(
					"VariantJson: resource '%s' has no source or virtual path; saving an empty resource path.\n",
					resource->GetName().ToUtf8().c_str()));
				outJson = "";
				return true;
			}

			outJson = resourcePath;
			return true;
		}

		return false;
	}

	case Variant::Type::Empty:
		outJson = nullptr;
		return true;
	default:
		return false;
	}
}

bool TryDeserialize(Json const& json, Variant::Type expectedType, Variant& outValue)
{
	try
	{
		switch (expectedType)
		{
		case Variant::Type::Bool:
			if (json.is_boolean())
			{
				outValue = Variant(json.get<bool>());
				return true;
			}

			break;
		case Variant::Type::Int64:
			if (json.is_number_integer()
				&& (!json.is_number_unsigned() || json.get<uint64_t>() <= static_cast<uint64_t>((std::numeric_limits<int64_t>::max)())))
			{
				outValue = Variant(json.get<int64_t>());
				return true;
			}

			break;
		case Variant::Type::UInt32:
			if (json.is_number_unsigned() || (json.is_number_integer() && json.get<int64_t>() >= 0))
			{
				auto number = json.get<uint64_t>();
				if (number <= (std::numeric_limits<uint32_t>::max)())
				{
					outValue = Variant(static_cast<uint32_t>(number));
					return true;
				}
			}
			break;
		case Variant::Type::Int:
			if (json.is_number_integer() && json.get<double>() >= (std::numeric_limits<int>::min)() && json.get<double>() <= (std::numeric_limits<int>::max)())
			{
				outValue = Variant(json.get<int>());
				return true;
			}

			break;
		case Variant::Type::Float:
			if (json.is_number() && std::isfinite(json.get<float>()))
			{
				outValue = Variant(json.get<float>());
				return true;
			}

			break;
		case Variant::Type::String:
			if (json.is_string())
			{
				outValue = Variant(json.get<std::string>());
				return true;
			}

			break;
		case Variant::Type::Vector4:
			if (IsNumberArray(json, 4))
			{
				outValue = Variant(Vector4(json[0].get<float>(), json[1].get<float>(), json[2].get<float>(), json[3].get<float>()));
				return true;
			}

			break;
		case Variant::Type::AABB2:
			if (IsNumberArray(json, 4))
			{
				outValue = Variant(AABB2(json[0].get<float>(), json[1].get<float>(), json[2].get<float>(), json[3].get<float>()));
				return true;
			}

			break;
		case Variant::Type::OBB2:
			if (IsNumberArray(json, 6))
			{
				outValue = Variant(OBB2(Vector2(json[0].get<float>(), json[1].get<float>()), Vector2(json[2].get<float>(), json[3].get<float>()), Vector2(json[4].get<float>(), json[5].get<float>())));
				return true;
			}

			break;
		case Variant::Type::Capsule3:
			if (IsNumberArray(json, 7))
			{
				outValue = Variant(Capsule3(Vector3(json[0].get<float>(), json[1].get<float>(), json[2].get<float>()), Vector3(json[3].get<float>(), json[4].get<float>(), json[5].get<float>()), json[6].get<float>()));
				return true;
			}

			break;
		case Variant::Type::Vector2:
			if (IsNumberArray(json, 2))
			{
				outValue = Variant(Vector2(json[0].get<float>(), json[1].get<float>()));
				return true;
			}

			break;
		case Variant::Type::Vector3:
			if (IsNumberArray(json, 3))
			{
				outValue = Variant(Vector3(json[0].get<float>(), json[1].get<float>(), json[2].get<float>()));
				return true;
			}

			break;
		case Variant::Type::EulerAngles:
			if (IsNumberArray(json, 3))
			{
				outValue = Variant(EulerAngles(json[0].get<float>(), json[1].get<float>(), json[2].get<float>()));
				return true;
			}

			break;
		case Variant::Type::Matrix4x4:
			if (IsNumberArray(json, 16))
			{
				float matrixValues[16] = {};
				for (int index = 0; index < 16; ++index)
				{
					matrixValues[index] = json[index].get<float>();
				}

				outValue = Variant(Matrix4x4(matrixValues));
				return true;
			}

			break;
		case Variant::Type::Color:
			if (IsNumberArray(json, 4))
			{
				for (Json const& component : json)
				{
					if (!component.is_number_integer() || component.get<double>() < 0.0 || component.get<double>() > 255.0)
					{
						return false;
					}
				}

				outValue = Variant(
					Color(json[0].get<uint8_t>(), json[1].get<uint8_t>(), json[2].get<uint8_t>(), json[3].get<uint8_t>()));
				return true;
			}

			break;
		case Variant::Type::ObjectPtr:
			if (json.is_null())
			{
				Object* object = nullptr;
				outValue       = Variant(object);
				return true;
			}

			if (json.is_string())
			{
				VirtualPath resourcePath;
				if (!VirtualPath::TryParse(json.get<std::string>(), resourcePath))
				{
					break;
				}

				Ref<Resource> resource = ResourceLoader::Load(resourcePath);
				if (resource.IsValid())
				{
					outValue = resource;
					return true;
				}
			}

			break;
		case Variant::Type::Empty:
			if (json.is_null())
			{
				outValue = Variant();
				return true;
			}

			break;
		default:
			break;
		}
	}

	catch (std::exception const&)
	{
		return false;
	}

	return false;
}
} // namespace VariantJson
