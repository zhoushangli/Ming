#include "MingEngine/Scene/Core/PackedScene.hpp"

#include "MingEngine/Scene/Core/ClassDatabase.hpp"

#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Core/StringUtils.hpp"

#include "ThirdParty/nlohmann/json.hpp"

#include <cstdlib>
#include <exception>
#include <fstream>
#include <sstream>

namespace
{
unsigned int const kInvalidNodeId = 0xffffffffu;

std::string ToString(float value)
{
	std::ostringstream stream;
	stream << value;
	return stream.str();
}

Strings GetFunctionParts(std::string const& text, std::string const& functionName, int expectedCount)
{
	std::string prefix = functionName + "(";
	if (text.rfind(prefix, 0) != 0 || text.back() != ')')
	{
		ERROR_AND_DIE(Stringf("Invalid %s property value: %s", functionName.c_str(), text.c_str()));
	}

	std::string arguments = text.substr(prefix.size(), text.size() - prefix.size() - 1);
	Strings     parts     = SplitStringOnDelimiter(arguments, ',');
	if (static_cast<int>(parts.size()) != expectedCount)
	{
		ERROR_AND_DIE(Stringf("Invalid %s property value: %s", functionName.c_str(), text.c_str()));
	}

	return parts;
}

std::string SerializePropertyValue(ClassDatabase::PropertyValue const& value)
{
	if (std::holds_alternative<bool>(value))
	{
		return std::get<bool>(value) ? "true" : "false";
	}
	if (std::holds_alternative<int>(value))
	{
		return std::to_string(std::get<int>(value));
	}
	if (std::holds_alternative<float>(value))
	{
		return ToString(std::get<float>(value));
	}
	if (std::holds_alternative<std::string>(value))
	{
		return "\"" + std::get<std::string>(value) + "\"";
	}
	if (std::holds_alternative<Vec3>(value))
	{
		Vec3 const& vec = std::get<Vec3>(value);
		return "Vec3(" + ToString(vec.x) + ", " + ToString(vec.y) + ", " + ToString(vec.z) + ")";
	}
	if (std::holds_alternative<EulerAngles>(value))
	{
		EulerAngles const& angles = std::get<EulerAngles>(value);
		return "EulerAngles(" + ToString(angles.m_yawDegrees) + ", " + ToString(angles.m_pitchDegrees) + ", "
			   + ToString(angles.m_rollDegrees) + ")";
	}
	if (std::holds_alternative<Matrix4x4>(value))
	{
		float const*       matrix = std::get<Matrix4x4>(value).GetAsFloatArray();
		std::ostringstream stream;
		stream << "Matrix4x4(";
		for (int valueIndex = 0; valueIndex < 16; ++valueIndex)
		{
			if (valueIndex > 0)
			{
				stream << ", ";
			}
			stream << ToString(matrix[valueIndex]);
		}
		stream << ")";
		return stream.str();
	}

	ERROR_AND_DIE("PackedScene: unknown property value type.");
}

ClassDatabase::PropertyValue DeserializePropertyValue(std::string const& text)
{
	if (text == "true")
	{
		return true;
	}
	if (text == "false")
	{
		return false;
	}
	if (text.size() >= 2 && text.front() == '"' && text.back() == '"')
	{
		return text.substr(1, text.size() - 2);
	}
	if (text.rfind("Vec3(", 0) == 0)
	{
		Strings parts = GetFunctionParts(text, "Vec3", 3);
		return Vec3(
			static_cast<float>(atof(parts[0].c_str())),
			static_cast<float>(atof(parts[1].c_str())),
			static_cast<float>(atof(parts[2].c_str()))
		);
	}
	if (text.rfind("EulerAngles(", 0) == 0)
	{
		Strings parts = GetFunctionParts(text, "EulerAngles", 3);
		return EulerAngles(
			static_cast<float>(atof(parts[0].c_str())),
			static_cast<float>(atof(parts[1].c_str())),
			static_cast<float>(atof(parts[2].c_str()))
		);
	}
	if (text.rfind("Matrix4x4(", 0) == 0)
	{
		Strings parts  = GetFunctionParts(text, "Matrix4x4", 16);
		float   values[16] = {};
		for (int valueIndex = 0; valueIndex < 16; ++valueIndex)
		{
			values[valueIndex] = static_cast<float>(atof(parts[valueIndex].c_str()));
		}
		return Matrix4x4(values);
	}

	if (text.find('.') != std::string::npos)
	{
		return static_cast<float>(atof(text.c_str()));
	}

	return atoi(text.c_str());
}

bool ArePropertyValuesEqual(ClassDatabase::PropertyValue const& left, ClassDatabase::PropertyValue const& right)
{
	if (left.index() != right.index())
	{
		return false;
	}

	if (std::holds_alternative<bool>(left))
	{
		return std::get<bool>(left) == std::get<bool>(right);
	}
	if (std::holds_alternative<int>(left))
	{
		return std::get<int>(left) == std::get<int>(right);
	}
	if (std::holds_alternative<float>(left))
	{
		return std::get<float>(left) == std::get<float>(right);
	}
	if (std::holds_alternative<std::string>(left))
	{
		return std::get<std::string>(left) == std::get<std::string>(right);
	}
	if (std::holds_alternative<Vec3>(left))
	{
		return std::get<Vec3>(left) == std::get<Vec3>(right);
	}
	if (std::holds_alternative<EulerAngles>(left))
	{
		EulerAngles const& a = std::get<EulerAngles>(left);
		EulerAngles const& b = std::get<EulerAngles>(right);
		return a.m_yawDegrees == b.m_yawDegrees && a.m_pitchDegrees == b.m_pitchDegrees
			   && a.m_rollDegrees == b.m_rollDegrees;
	}
	if (std::holds_alternative<Matrix4x4>(left))
	{
		float const* a = std::get<Matrix4x4>(left).GetAsFloatArray();
		float const* b = std::get<Matrix4x4>(right).GetAsFloatArray();
		for (int valueIndex = 0; valueIndex < 16; ++valueIndex)
		{
			if (a[valueIndex] != b[valueIndex])
			{
				return false;
			}
		}
		return true;
	}

	return false;
}
} // namespace

PackedScene::PackedProperty::PackedProperty(std::string const& name, ClassDatabase::PropertyValue const& value)
	: m_name(name)
	, m_value(value)
{
}

std::string PackedScene::PackedProperty::Serialize() const
{
	return m_name + ": " + SerializePropertyValue(m_value);
}

PackedScene::PackedProperty PackedScene::PackedProperty::Deserialize(std::string const& text)
{
	size_t separatorIndex = text.find(": ");
	if (separatorIndex == std::string::npos)
	{
		ERROR_AND_DIE(Stringf("Invalid packed property: %s", text.c_str()));
	}

	std::string name      = text.substr(0, separatorIndex);
	std::string valueText = text.substr(separatorIndex + 2);
	if (name.empty() || valueText.empty())
	{
		ERROR_AND_DIE(Stringf("Invalid packed property: %s", text.c_str()));
	}

	return PackedProperty(name, DeserializePropertyValue(valueText));
}

bool PackedScene::PackedProperty::CanApplyTo(ClassDatabase::PropertyInfo const& propertyInfo) const
{
	if (!propertyInfo.m_setter)
	{
		return false;
	}

	switch (propertyInfo.m_type)
	{
	case ClassDatabase::PropertyType::Bool:
		return std::holds_alternative<bool>(m_value);
	case ClassDatabase::PropertyType::Int:
		return std::holds_alternative<int>(m_value);
	case ClassDatabase::PropertyType::Float:
		return std::holds_alternative<float>(m_value);
	case ClassDatabase::PropertyType::String:
		return std::holds_alternative<std::string>(m_value);
	case ClassDatabase::PropertyType::Vec3:
		return std::holds_alternative<Vec3>(m_value);
	case ClassDatabase::PropertyType::EulerAngles:
		return std::holds_alternative<EulerAngles>(m_value);
	case ClassDatabase::PropertyType::Matrix4x4:
		return std::holds_alternative<Matrix4x4>(m_value);
	}

	return false;
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
	Node*              root = nullptr;
	std::vector<Node*> nodes;
	nodes.reserve(m_packedNodes.size());

	for (PackedNode const& packedNode : m_packedNodes)
	{
		Object* object = ClassDatabase::CreateInstance(packedNode.m_type);
		Node*   node   = dynamic_cast<Node*>(object);

		if (node == nullptr)
		{
			ERROR_AND_DIE(Stringf("Type %s is not a subclass of Node.", packedNode.m_type.c_str()));
			return nullptr;
		}

		node->SetName(packedNode.m_name);

		if (packedNode.m_parentIndex == kInvalidNodeId)
		{
			root = node;
		}
		else
		{
			// When we store the PackedScene
			// we guarantee that parent nodes are always stored first
			nodes[packedNode.m_parentIndex]->AddNode(node);
		}

		for (PackedScene::PackedProperty const& runtimeProperty : packedNode.m_properties)
		{
			ClassDatabase::PropertyInfo const* property =
				ClassDatabase::FindProperty(packedNode.m_type, runtimeProperty.m_name);
			if (property == nullptr)
			{
				DebuggerPrintf(
					"PackedScene: skipping unknown property '%s' on type '%s'.\n",
					runtimeProperty.m_name.c_str(),
					packedNode.m_type.c_str()
				);
				continue;
			}

			if (!runtimeProperty.CanApplyTo(*property))
			{
				DebuggerPrintf(
					"PackedScene: skipping incompatible property '%s' on type '%s'.\n",
					runtimeProperty.m_name.c_str(),
					packedNode.m_type.c_str()
				);
				continue;
			}

			try
			{
				property->m_setter(*node, runtimeProperty.m_value);
			}
			catch (std::exception const&)
			{
				DebuggerPrintf(
					"PackedScene: failed to apply property '%s' on type '%s'.\n",
					runtimeProperty.m_name.c_str(),
					packedNode.m_type.c_str()
				);
			}
		}
		nodes.push_back(node);
	}

	return root;
}

bool PackedScene::SaveToFile(std::string const& filename) const
{
	if (filename.empty())
	{
		return false;
	}

	nlohmann::ordered_json root;
	root["version"] = 1;
	root["nodes"]   = nlohmann::json::array();

	for (PackedNode const& node : m_packedNodes)
	{
		nlohmann::ordered_json nodeJson;
		nodeJson["id"]   = node.m_id;
		nodeJson["name"] = node.m_name;
		nodeJson["type"] = node.m_type;

		if (node.m_parentIndex == kInvalidNodeId)
		{
			nodeJson["parent"] = nullptr;
		}
		else
		{
			nodeJson["parent"] = node.m_parentIndex;
		}

		nodeJson["properties"] = nlohmann::json::array();
		for (PackedProperty const& property : node.m_properties)
		{
			nodeJson["properties"].push_back(property.Serialize());
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
	catch (nlohmann::json::parse_error const& error)
	{
		ERROR_AND_DIE(Stringf("Failed to serialize PackedScene to JSON: %s", error.what()));
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

	nlohmann::ordered_json root;
	try
	{
		input >> root;
	}
	catch (nlohmann::json::parse_error const& error)
	{
		ERROR_AND_DIE(Stringf("Failed to deserialize PackedScene from JSON: %s", error.what()));
		return false;
	}

	if (!root.contains("version") || !root.contains("nodes"))
	{
		return false;
	}

	m_packedNodes.clear();
	for (auto const& nodeJson : root["nodes"])
	{
		PackedNode packedNode;
		packedNode.m_id   = nodeJson["id"].get<unsigned int>();
		packedNode.m_name = nodeJson["name"].get<std::string>();
		packedNode.m_type = nodeJson["type"].get<std::string>();
		packedNode.m_parentIndex =
			nodeJson["parent"].is_null() ? kInvalidNodeId : nodeJson["parent"].get<unsigned int>();

		for (auto const& propertyJson : nodeJson["properties"])
		{
			if (!propertyJson.is_string())
			{
				ERROR_AND_DIE("PackedScene: property entries must be serialized strings.");
			}

			packedNode.m_properties.push_back(PackedProperty::Deserialize(propertyJson.get<std::string>()));
		}

		m_packedNodes.push_back(packedNode);
	}

	return true;
}

void PackedScene::ParseNodeRecursively(Node const* node, std::vector<PackedNode>& outNodes)
{
	if (node == nullptr || !node->IsSerializable())
	{
		return;
	}

	PackedNode packedNode;
	packedNode.m_name = node->GetName();
	packedNode.m_type = node->GetClassName();
	packedNode.m_id   = m_nextNodeId++;

	m_nodeToId[node] = packedNode.m_id;

	unsigned int parentId = kInvalidNodeId;
	Node const*  parent   = node->GetParent();
	if (parent != nullptr)
	{
		auto iter = m_nodeToId.find(parent);
		if (iter != m_nodeToId.end())
		{
			parentId = iter->second;
		}
	}
	packedNode.m_parentIndex = parentId;

	Object* defaultObject = ClassDatabase::CreateInstance(packedNode.m_type);
	Node*   defaultNode   = dynamic_cast<Node*>(defaultObject);
	bool    hasDefault    = defaultNode != nullptr;
	if (defaultObject != nullptr && !hasDefault)
	{
		DebuggerPrintf("PackedScene: default object for type '%s' is not a Node.\n", packedNode.m_type.c_str());
	}

	std::vector<ClassDatabase::PropertyInfo const*> properties = ClassDatabase::GetAllProperties(packedNode.m_type);
	for (ClassDatabase::PropertyInfo const* property : properties)
	{
		if (property == nullptr || !property->m_getter)
		{
			continue;
		}

		try
		{
			ClassDatabase::PropertyValue value = property->m_getter(*node);
			if (hasDefault)
			{
				ClassDatabase::PropertyValue defaultValue = property->m_getter(*defaultNode);
				if (ArePropertyValuesEqual(value, defaultValue))
				{
					continue;
				}
			}

			packedNode.m_properties.emplace_back(property->m_name, value);
		}
		catch (std::exception const&)
		{
			DebuggerPrintf(
				"PackedScene: failed to read property '%s' on type '%s'.\n",
				property->m_name.c_str(),
				packedNode.m_type.c_str()
			);
		}
	}

	delete defaultObject;

	outNodes.push_back(packedNode);

	std::vector<Node*> const& children = node->GetChildren();
	for (Node const* child : children)
	{
		ParseNodeRecursively(child, outNodes);
	}
}
