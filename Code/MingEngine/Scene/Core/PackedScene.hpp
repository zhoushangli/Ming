#pragma once

#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodePath.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

#include <string>
#include <vector>

// When we seerialize a scene, we will go through the following steps:
// runtime scene ---> PackedScene ---> json file
// Pack() / Instantiate() will convert between runtime scene and PackedScene
// SaveToFile() / LoadFromFile() will convert between PackedScene and json file

struct PackedProperty
{
	PackedProperty() = default;
	PackedProperty(std::string const& name, Variant const& value);

	bool CanApplyTo(PropertyInfo const& propertyInfo) const;

	std::string m_name;
	Variant     m_value;
};

struct PackedNode
{
	std::string m_name;
	std::string m_type;
	std::string m_parentPath;
	std::vector<PackedProperty> m_properties;
};

struct PackedSceneData
{
	std::vector<PackedNode> m_packedNodes;
};

class PackedScene
{
public:
	PackedScene()  = default;
	~PackedScene() = default;

	bool  Pack(Node const* node);
	Node* Instantiate() const;

	bool SaveToFile(std::string const& filename) const;
	bool LoadFromFile(std::string const& filename);

protected:
	bool ParseNodeRecursively(
		Node const* node,
		std::string const& parentPath,
		std::vector<PackedNode>& outNodes);

protected:
	PackedSceneData m_data;
};

