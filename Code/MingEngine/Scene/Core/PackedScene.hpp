#pragma once

#include "MingEngine/Scene/Core/Node.hpp"

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

// When we seerialize a scene, we will go through the following steps:
// runtime scene ---> PackedScene ---> json file
// Pack() / Instantiate() will convert between runtime scene and PackedScene
// SaveToFile() / LoadFromFile() will convert between PackedScene and json file
class PackedScene
{
protected:
	struct PackedProperty
	{
		PackedProperty() = default;
		PackedProperty(std::string const& name, ClassDatabase::PropertyValue const& value);

		std::string Serialize() const;
		static PackedProperty Deserialize(std::string const& text);
		bool        CanApplyTo(ClassDatabase::PropertyInfo const& propertyInfo) const;

		std::string                  m_name;
		ClassDatabase::PropertyValue m_value;
	};

	struct PackedNode
	{
		std::string m_name;
		std::string m_type;
		// The ID here refers to the index of the node in the PackedScene's m_nodes vector
		// It is not a unique identifier. (Like node handle ID)
		unsigned int                m_id          = 0;
		unsigned int                m_parentIndex = 0;
		std::vector<PackedProperty> m_properties;
	};

public:
	PackedScene()  = default;
	~PackedScene() = default;

	bool  Pack(Node const* node);
	Node* Instantiate() const;

	bool SaveToFile(std::string const& filename) const;
	bool LoadFromFile(std::string const& filename);

protected:
	void ParseNodeRecursively(Node const* node, std::vector<PackedNode>& outNodes);

protected:
	std::vector<PackedNode> m_packedNodes;

	unsigned int                                  m_nextNodeId = 0;
	std::unordered_map<Node const*, unsigned int> m_nodeToId;
};
