#pragma once

#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Object/Variant.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodePath.hpp"

#include <string>
#include <vector>

class PackedSceneLoader;
class PackedSceneSaver;

// When we serialize a scene, we go through the following steps:
// runtime scene <-> PackedScene <-> .mscn file
// Pack() / Instantiate() convert between runtime scene and PackedScene.
// PackedSceneLoader / PackedSceneSaver convert between PackedScene and .mscn files.

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
	std::string                 m_name;
	std::string                 m_type;
	std::string                 m_parentPath;
	std::vector<PackedProperty> m_properties;
};

struct PackedSceneData
{
	std::vector<PackedNode> m_packedNodes;
};

class PackedScene : public Resource
{
	MCLASS(PackedScene, Resource)

	friend class PackedSceneLoader;
	friend class PackedSceneSaver;

public:
	PackedScene()  = default;
	~PackedScene() = default;

	bool  Pack(Node const* node);
	Node* Instantiate() const;
	bool  CopyFrom(Resource const& other) override;

protected:
	static void BindMethods() {};

	bool ParseNodeRecursively(Node const* node, std::string const& parentPath, std::vector<PackedNode>& outNodes);

protected:
	PackedSceneData m_data;
};