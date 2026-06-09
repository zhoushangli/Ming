#include "MingEngine/Scene/Core/PackedScene.hpp"

#include "MingEngine/Scene/Core/ClassDatabase.hpp"

#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Core/StringUtils.hpp"

#include "ThirdParty/nlohmann/json.hpp"

#include <exception>
#include <fstream>

namespace
{
unsigned int const kInvalidNodeId = 0xffffffffu;
using Json                        = nlohmann::ordered_json;

Json SerializeVariant(Variant const& value)
{
	switch (value.GetType())
	{
	case Variant::Type::Bool:
		return value.As<bool>();
	case Variant::Type::Int:
		return value.As<int>();
	case Variant::Type::Float:
		return value.As<float>();
	case Variant::Type::String:
		return value.As<std::string>();
	case Variant::Type::Vec3:
	{
		Vec3 const& vector = value.As<Vec3>();
		return Json::array({ vector.x, vector.y, vector.z });
	}
	case Variant::Type::EulerAngles:
	{
		EulerAngles const& angles = value.As<EulerAngles>();
		return Json::array({ angles.m_yawDegrees, angles.m_pitchDegrees, angles.m_rollDegrees });
	}
	case Variant::Type::Matrix4x4:
	{
		Json         result = Json::array();
		float const* matrix = value.As<Matrix4x4>().GetAsFloatArray();
		for (int index = 0; index < 16; ++index)
		{
			result.push_back(matrix[index]);
		}
		return result;
	}
	case Variant::Type::Empty:
		return nullptr;
	}

	return nullptr;
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
				outValue =
					Variant(EulerAngles(json[0].get<float>(), json[1].get<float>(), json[2].get<float>()));
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
		case Variant::Type::Empty:
			if (json.is_null())
			{
				outValue = Variant();
				return true;
			}
			break;
		}
	}
	catch (std::exception const&)
	{
		return false;
	}

	return false;
}
} // namespace

PackedScene::PackedProperty::PackedProperty(std::string const& name, Variant const& value)
	: m_name(name)
	, m_value(value)
{
}

bool PackedScene::PackedProperty::CanApplyTo(ClassDatabase::PropertyInfo const& propertyInfo) const
{
	return propertyInfo.GetSetter() != nullptr && propertyInfo.m_type == m_value.GetType();
}

bool PackedScene::Pack(Node const* node)
{
	m_packedNodes.clear();
	m_nodeToId.clear();
	m_nextNodeId = 0;

	if (node == nullptr)
	{
		return false;
	}

	ParseNodeRecursively(node, m_packedNodes);
	return true;
}

Node* PackedScene::Instantiate() const
{
	if (m_packedNodes.empty())
	{
		return nullptr;
	}

	Node*              root = nullptr;
	std::vector<Node*> nodes;
	nodes.reserve(m_packedNodes.size());

	// Phase 1: create every node and rebuild the detached scene hierarchy.
	// Packed nodes are parent-first, so each parent exists before its children are attached.
	for (PackedNode const& packedNode : m_packedNodes)
	{
		Object* object = ClassDatabase::CreateInstance(packedNode.m_type);
		Node*   node   = dynamic_cast<Node*>(object);
		if (node == nullptr)
		{
			delete object;
			delete root;
			DebuggerPrintf("PackedScene: type '%s' is not a creatable Node.\n", packedNode.m_type.c_str());
			return nullptr;
		}

		node->SetName(packedNode.m_name);
		if (packedNode.m_parentIndex == kInvalidNodeId)
		{
			if (root != nullptr)
			{
				delete node;
				delete root;
				DebuggerPrintf("PackedScene: scene contains more than one root node.\n");
				return nullptr;
			}
			root = node;
		}
		else
		{
			if (packedNode.m_parentIndex >= nodes.size())
			{
				delete node;
				delete root;
				DebuggerPrintf("PackedScene: node '%s' has an invalid parent index.\n", packedNode.m_name.c_str());
				return nullptr;
			}
			nodes[packedNode.m_parentIndex]->AddNode(node);
		}
		nodes.push_back(node);
	}

	// Phase 2: apply properties after the complete hierarchy exists. Setters may safely
	// inspect parents or children, while SceneTree lifecycle callbacks have not started yet.
	for (size_t nodeIndex = 0; nodeIndex < m_packedNodes.size(); ++nodeIndex)
	{
		PackedNode const& packedNode = m_packedNodes[nodeIndex];
		Node*             node       = nodes[nodeIndex];
		for (PackedProperty const& packedProperty : packedNode.m_properties)
		{
			ClassDatabase::PropertyInfo const* property =
				ClassDatabase::FindProperty(packedNode.m_type, packedProperty.m_name);
			if (property == nullptr)
			{
				DebuggerPrintf("PackedScene: skipping unknown property '%s' on type '%s'.\n",
					packedProperty.m_name.c_str(),
					packedNode.m_type.c_str());
				continue;
			}
			if (!packedProperty.CanApplyTo(*property))
			{
				DebuggerPrintf("PackedScene: skipping incompatible property '%s' on type '%s'.\n",
					packedProperty.m_name.c_str(),
					packedNode.m_type.c_str());
				continue;
			}

			try
			{
				property->GetSetter()->Invoke(*node, { packedProperty.m_value });
			}
			catch (std::exception const& error)
			{
				DebuggerPrintf("PackedScene: failed to apply property '%s' on type '%s': %s\n",
					packedProperty.m_name.c_str(),
					packedNode.m_type.c_str(),
					error.what());
			}
		}
	}

	return root;
}

bool PackedScene::SaveToFile(std::string const& filename) const
{
	if (filename.empty())
	{
		return false;
	}

	Json root;
	root["version"] = 1;
	root["nodes"]   = Json::array();

	for (PackedNode const& node : m_packedNodes)
	{
		Json nodeJson;
		nodeJson["id"]     = node.m_id;
		nodeJson["name"]   = node.m_name;
		nodeJson["type"]   = node.m_type;
		nodeJson["parent"] = node.m_parentIndex == kInvalidNodeId
								 ? Json(nullptr)
								 : Json(node.m_parentIndex);

		nodeJson["properties"] = Json::object();
		for (PackedProperty const& property : node.m_properties)
		{
			nodeJson["properties"][property.m_name] = SerializeVariant(property.m_value);
		}
		root["nodes"].push_back(nodeJson);
	}

	std::ofstream output(filename);
	if (!output.is_open())
	{
		return false;
	}

	try
	{
		output << root.dump(4);
	}
	catch (std::exception const& error)
	{
		DebuggerPrintf("PackedScene: failed to save '%s': %s\n", filename.c_str(), error.what());
		return false;
	}

	return output.good();
}

bool PackedScene::LoadFromFile(std::string const& filename)
{
	std::ifstream input(filename);
	if (!input.is_open())
	{
		return false;
	}

	Json root;
	try
	{
		input >> root;
		if (!root.contains("version") || root["version"].get<int>() != 1 || !root.contains("nodes")
			|| !root["nodes"].is_array())
		{
			return false;
		}

		std::vector<PackedNode> loadedNodes;
		for (Json const& nodeJson : root["nodes"])
		{
			if (!nodeJson.contains("id") || !nodeJson.contains("name") || !nodeJson.contains("type")
				|| !nodeJson.contains("parent") || !nodeJson.contains("properties")
				|| !nodeJson["properties"].is_object())
			{
				return false;
			}

			PackedNode packedNode;
			packedNode.m_id   = nodeJson["id"].get<unsigned int>();
			packedNode.m_name = nodeJson["name"].get<std::string>();
			packedNode.m_type = nodeJson["type"].get<std::string>();
			packedNode.m_parentIndex =
				nodeJson["parent"].is_null() ? kInvalidNodeId : nodeJson["parent"].get<unsigned int>();

			for (auto propertyEntry = nodeJson["properties"].begin();
				 propertyEntry != nodeJson["properties"].end();
				 ++propertyEntry)
			{
				std::string const propertyName = propertyEntry.key();
				ClassDatabase::PropertyInfo const* property =
					ClassDatabase::FindProperty(packedNode.m_type, propertyName);
				if (property == nullptr)
				{
					DebuggerPrintf("PackedScene: skipping unknown property '%s' on type '%s'.\n",
						propertyName.c_str(),
						packedNode.m_type.c_str());
					continue;
				}

				Variant value;
				if (!TryDeserializeVariant(propertyEntry.value(), property->m_type, value))
				{
					DebuggerPrintf("PackedScene: skipping property '%s' with an incompatible JSON value on type '%s'.\n",
						propertyName.c_str(),
						packedNode.m_type.c_str());
					continue;
				}
				packedNode.m_properties.emplace_back(propertyName, value);
			}
			loadedNodes.push_back(std::move(packedNode));
		}

		m_packedNodes = std::move(loadedNodes);
	}
	catch (std::exception const& error)
	{
		DebuggerPrintf("PackedScene: failed to load '%s': %s\n", filename.c_str(), error.what());
		return false;
	}

	return true;
}

void PackedScene::ParseNodeRecursively(Node const* node, std::vector<PackedNode>& outNodes)
{
	if (node == nullptr || !node->GetSerializable())
	{
		return;
	}

	PackedNode packedNode;
	packedNode.m_name = node->GetName();
	packedNode.m_type = node->GetClassName();
	packedNode.m_id   = m_nextNodeId++;
	m_nodeToId[node]  = packedNode.m_id;

	Node const* parent = node->GetParent();
	auto const  parentEntry = parent != nullptr ? m_nodeToId.find(parent) : m_nodeToId.end();
	packedNode.m_parentIndex = parentEntry != m_nodeToId.end() ? parentEntry->second : kInvalidNodeId;

	Object* defaultObject = ClassDatabase::CreateInstance(packedNode.m_type);
	Node*   defaultNode   = dynamic_cast<Node*>(defaultObject);
	if (defaultObject != nullptr && defaultNode == nullptr)
	{
		DebuggerPrintf("PackedScene: default object for type '%s' is not a Node.\n", packedNode.m_type.c_str());
	}

	std::vector<ClassDatabase::PropertyInfo const*> properties = ClassDatabase::GetAllProperties(packedNode.m_type);
	for (ClassDatabase::PropertyInfo const* property : properties)
	{
		if (property == nullptr
			|| !property->HasUsage(ClassDatabase::PropertyInfo::PropertyUsageFlags::Storage)
			|| property->GetGetter() == nullptr)
		{
			continue;
		}

		try
		{
			Variant value = property->GetGetter()->Invoke(const_cast<Node&>(*node), {});
			if (defaultNode != nullptr)
			{
				Variant defaultValue = property->GetGetter()->Invoke(*defaultNode, {});
				if (value == defaultValue)
				{
					continue;
				}
			}
			packedNode.m_properties.emplace_back(property->m_name, value);
		}
		catch (std::exception const& error)
		{
			DebuggerPrintf("PackedScene: failed to read property '%s' on type '%s': %s\n",
				property->m_name.c_str(),
				packedNode.m_type.c_str(),
				error.what());
		}
	}

	delete defaultObject;
	outNodes.push_back(std::move(packedNode));

	for (Node const* child : node->GetChildren())
	{
		ParseNodeRecursively(child, outNodes);
	}
}
