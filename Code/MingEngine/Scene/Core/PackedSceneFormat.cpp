#include "MingEngine/Scene/Core/PackedSceneFormat.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"

#include "ThirdParty/nlohmann/json.hpp"

#include <exception>
#include <string>

namespace
{
using Json = nlohmann::ordered_json;

bool HasExtension(std::string const& virtualPath, std::string const& extension)
{
	return virtualPath.size() >= extension.size()
		   && virtualPath.compare(virtualPath.size() - extension.size(), extension.size(), extension) == 0;
}

bool TrySerializeVariant(Variant const& value, Json& outJson)
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
	case Variant::Type::ObjectPtr:
	{
		Object* object = value.As<Object*>();
		if (object == nullptr)
		{
			outJson = nullptr;
			return true;
		}

		Resource const* resource = dynamic_cast<Resource const*>(object);
		if (resource != nullptr && !resource->GetVirtualPath().empty())
		{
			outJson = resource->GetVirtualPath();
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

bool IsNumberArray(Json const& json, size_t expectedSize)
{
	if (!json.is_array() || json.size() != expectedSize)
	{
		return false;
	}

	for (Json const& entry : json)
	{
		if (!entry.is_number())
		{
			return false;
		}
	}
	return true;
}

bool TryDeserializeVariant(Json const& json, Variant::Type expectedType, Variant& outValue)
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

bool TryParseProperties(Json const& nodeJson, PackedNode& outNode)
{
	if (!nodeJson.contains("properties"))
	{
		return true;
	}

	Json const& propertiesJson = nodeJson["properties"];
	if (!propertiesJson.is_object())
	{
		return false;
	}

	for (auto propertyEntry = propertiesJson.begin(); propertyEntry != propertiesJson.end(); ++propertyEntry)
	{
		std::string const   propertyName = propertyEntry.key();
		PropertyInfo const* property     = ClassDatabase::FindProperty(outNode.m_type, propertyName);
		if (property == nullptr)
		{
			DebuggerPrintf(
				"PackedScene: skipping unknown property '%s' on type '%s'.\n",
				propertyName.c_str(),
				outNode.m_type.c_str());
			continue;
		}

		Variant value;
		if (!TryDeserializeVariant(propertyEntry.value(), property->m_type, value))
		{
			DebuggerPrintf(
				"PackedScene: skipping property '%s' with an incompatible JSON value on type '%s'.\n",
				propertyName.c_str(),
				outNode.m_type.c_str());
			continue;
		}

		outNode.m_properties.emplace_back(propertyName, value);
	}

	return true;
}

bool TryParseNode(Json const& nodeJson, PackedSceneData& sceneData)
{
	if (!nodeJson.contains("name") || !nodeJson["name"].is_string() || !nodeJson.contains("type")
		|| !nodeJson["type"].is_string())
	{
		DebuggerPrintf("PackedScene: every node must contain string fields 'name' and 'type'.\n");
		return false;
	}

	PackedNode packedNode;
	packedNode.m_name = nodeJson["name"].get<std::string>();
	packedNode.m_type = nodeJson["type"].get<std::string>();
	if (packedNode.m_name.empty() || packedNode.m_name == "." || packedNode.m_name.find('/') != std::string::npos)
	{
		DebuggerPrintf(
			"PackedScene: node name '%s' cannot be represented in a scene path.\n",
			packedNode.m_name.c_str());
		return false;
	}

	if (!nodeJson.contains("parent") || !nodeJson["parent"].is_string())
	{
		return false;
	}

	packedNode.m_parentPath = nodeJson["parent"].get<std::string>();

	if (!TryParseProperties(nodeJson, packedNode))
	{
		return false;
	}

	sceneData.m_packedNodes.push_back(std::move(packedNode));
	return true;
}

bool TryParseScene(Json const& root, PackedSceneData& outData)
{
	if (!root.contains("nodes") || !root["nodes"].is_array() || root["nodes"].empty())
	{
		DebuggerPrintf("PackedScene: the root JSON must contain a non-empty 'nodes' array.\n");
		return false;
	}

	Json const& nodesJson = root["nodes"];
	for (size_t nodeIndex = 0; nodeIndex < nodesJson.size(); ++nodeIndex)
	{
		if (!TryParseNode(nodesJson[nodeIndex], outData))
		{
			return false;
		}
	}

	return true;
}
} // namespace

std::vector<std::string> PackedSceneLoader::GetSupportedExtensions() const
{
	return std::vector<std::string>({ ".mscn" });
}

Ref<Resource> PackedSceneLoader::Load(const std::string& virtualPath)
{
	std::string relativePath;
	if (!FileSystem::TryGetRelativePath(virtualPath, relativePath) || g_engine == nullptr
		|| g_engine->m_fileSystem == nullptr)
	{
		return Ref<Resource>();
	}

	std::string text;
	if (!g_engine->m_fileSystem->ReadText(virtualPath, text))
	{
		return Ref<Resource>();
	}

	try
	{
		Json root = Json::parse(text);

		PackedSceneData loadedData;
		if (!TryParseScene(root, loadedData))
		{
			return Ref<Resource>();
		}

		Ref<PackedScene> packedScene = Ref<PackedScene>(new PackedScene());
		packedScene->m_data          = std::move(loadedData);
		packedScene->SetVirtualPath(virtualPath);

		size_t      slash = relativePath.find_last_of('/');
		std::string name  = slash == std::string::npos ? relativePath : relativePath.substr(slash + 1);
		packedScene->SetName(name);
		return packedScene;
	}
	catch (std::exception const& error)
	{
		DebuggerPrintf("PackedScene: failed to load '%s': %s\n", virtualPath.c_str(), error.what());
		return Ref<Resource>();
	}
}

bool PackedSceneSaver::CanSave(std::string const& virtualPath, Variant const& value) const
{
	Ref<PackedScene> packedScene(value);
	return packedScene.IsValid() && HasExtension(virtualPath, ".mscn");
}

bool PackedSceneSaver::Save(std::string const& virtualPath, Variant const& value)
{
	Ref<PackedScene> packedScene(value);
	if (!packedScene.IsValid() || !FileSystem::IsVirtualPath(virtualPath) || g_engine == nullptr
		|| g_engine->m_fileSystem == nullptr)
	{
		return false;
	}

	Json root     = Json::object();
	root["nodes"] = Json::array();

	for (PackedNode const& node : packedScene->m_data.m_packedNodes)
	{
		Json nodeJson;
		nodeJson["name"]   = node.m_name;
		nodeJson["type"]   = node.m_type;
		nodeJson["parent"] = node.m_parentPath;

		if (!node.m_properties.empty())
		{
			nodeJson["properties"] = Json::object();
			for (PackedProperty const& property : node.m_properties)
			{
				Json propertyJson;
				if (TrySerializeVariant(property.m_value, propertyJson))
				{
					nodeJson["properties"][property.m_name] = propertyJson;
				}
				else
				{
					DebuggerPrintf("PackedScene: skipping unsavable property '%s'.\n", property.m_name.c_str());
				}
			}
		}

		root["nodes"].push_back(nodeJson);
	}

	try
	{
		std::string const text = root.dump(4);
		if (!g_engine->m_fileSystem->WriteText(virtualPath, text))
		{
			return false;
		}
		packedScene->SetVirtualPath(virtualPath);
		return true;
	}
	catch (std::exception const& error)
	{
		DebuggerPrintf("PackedScene: failed to save '%s': %s\n", virtualPath.c_str(), error.what());
		return false;
	}
}
