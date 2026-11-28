#pragma once

#include "MingEngine/Core/IO/Resource.hpp"
#include "MingEngine/Core/Object/Variant.hpp"
#include "MingEngine/Core/String.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/NodePath.hpp"

#include <vector>

class ResourceFormatLoaderText;
class ResourceFormatSaverText;

// Convert between runtime nodes, PackedScene data and text scene files.
// e.g. Pack(root), ResourceSaver::Save(path, scene), then Load(path)->Instantiate().

struct PackedProperty
{
public:
	PackedProperty() = default;
	PackedProperty(String const& name, Variant const& value);

	bool CanApplyTo(PropertyInfo const& propertyInfo) const;

public:
	String  m_name;
	Variant m_value;
};

struct PackedNode
{
	String                      m_name;
	String                      m_type;
	String                      m_parentPath;
	std::vector<PackedProperty> m_properties;
};

struct PackedSceneData
{
	std::vector<PackedNode> m_packedNodes;
};

class PackedScene : public Resource
{
	MCLASS(PackedScene, Resource)
	friend class ResourceFormatLoaderText;
	friend class ResourceFormatSaverText;

public:
	PackedScene()  = default;
	~PackedScene() = default;

	bool  Pack(Node const* node);
	Node* Instantiate() const;
	bool  MoveFrom(Resource&& other) override;

protected:
	static void BindMethods();

	bool ParseNodeRecursively(Node const* node, String const& parentPath, std::vector<PackedNode>& outNodes);

protected:
	PackedSceneData m_data;
};
