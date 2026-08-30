#include "MingEngine/Scene/Core/PackedSceneFormat.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/VariantJson.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"

#include "ThirdParty/nlohmann/json.hpp"

#include <exception>
#include <string>

namespace
{
using Json = nlohmann::ordered_json;

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
		if (!VariantJson::TryDeserialize(propertyEntry.value(), property->m_type, value))
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
	return std::vector<std::string>({ ".tscn" });
}

Ref<Resource> PackedSceneLoader::Load(VirtualPath const& virtualPath)
{
	if (!virtualPath.IsValid() || g_engine == nullptr
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

		Ref<PackedScene> packedScene = CreateRef<PackedScene>();
		packedScene->m_data          = std::move(loadedData);
		packedScene->SetVirtualPath(virtualPath);

		packedScene->SetName(virtualPath.GetFileName());
		return packedScene;
	}
	catch (std::exception const& error)
	{
		DebuggerPrintf("PackedScene: failed to load '%s': %s\n", virtualPath.CStr(), error.what());
		return Ref<Resource>();
	}
}

bool PackedSceneSaver::CanSave(VirtualPath const& virtualPath, Variant const& value) const
{
	Ref<PackedScene> packedScene(value);
	return packedScene.IsValid() && virtualPath.HasExtension(".tscn");
}

bool PackedSceneSaver::Save(VirtualPath const& virtualPath, Variant const& value)
{
	Ref<PackedScene> packedScene(value);
	if (!packedScene.IsValid() || !virtualPath.IsValid() || g_engine == nullptr
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
				if (VariantJson::TrySerialize(property.m_value, propertyJson))
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
		DebuggerPrintf("PackedScene: failed to save '%s': %s\n", virtualPath.CStr(), error.what());
		return false;
	}
}
