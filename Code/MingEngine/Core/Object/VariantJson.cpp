#include "MingEngine/Core/Object/VariantJson.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"

#include <exception>

namespace
{
std::string GetSerializableResourcePath(Resource const& resource)
{
	if (!resource.GetSourceFilePath().empty())
	{
		return resource.GetSourceFilePath();
	}

	return resource.GetVirtualPath();
}

bool IsNumberArray(VariantJson::Json const& json, size_t expectedSize)
{
	if (!json.is_array() || json.size() != expectedSize)
	{
		return false;
	}

	for (VariantJson::Json const& entry : json)
	{
		if (!entry.is_number())
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
	case Variant::Type::Int:
		outJson = value.As<int>();
		return true;
	case Variant::Type::Float:
		outJson = value.As<float>();
		return true;
	case Variant::Type::String:
		outJson = value.As<std::string>();
		return true;
	case Variant::Type::Vec3:
	{
		Vec3 const& vector = value.As<Vec3>();
		outJson            = Json::array({ vector.x, vector.y, vector.z });
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
				DebuggerPrintf(
					"VariantJson: resource '%s' has no source or virtual path; saving an empty resource path.\n",
					resource->GetName().c_str());
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
		case Variant::Type::Int:
			if (json.is_number_integer())
			{
				outValue = Variant(json.get<int>());
				return true;
			}
			break;
		case Variant::Type::Float:
			if (json.is_number())
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
		case Variant::Type::Vec3:
			if (IsNumberArray(json, 3))
			{
				outValue = Variant(Vec3(json[0].get<float>(), json[1].get<float>(), json[2].get<float>()));
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
				outValue = Variant(Color(json[0].get<float>(), json[1].get<float>(), json[2].get<float>(), json[3].get<float>()));
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
				Ref<Resource> resource = ResourceLoader::Load(json.get<std::string>());
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
