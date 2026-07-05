#include "MingEngine/Scene/Core/PackedScene.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include <exception>
PackedProperty::PackedProperty(std::string const& name, Variant const& value) : m_name(name), m_value(value) {}

bool PackedProperty::CanApplyTo(PropertyInfo const& propertyInfo) const
{
	return propertyInfo.GetSetter() != nullptr && propertyInfo.m_type == m_value.GetType();
}

bool PackedScene::Pack(Node const* node)
{
	m_data.m_packedNodes.clear();

	if (node == nullptr)
	{
		return false;
	}

	if (!ParseNodeRecursively(node, "", m_data.m_packedNodes))
	{
		m_data.m_packedNodes.clear();
		return false;
	}

	return !m_data.m_packedNodes.empty();
}

Node* PackedScene::Instantiate() const
{
	if (m_data.m_packedNodes.empty())
	{
		return nullptr;
	}

	Node*              root = nullptr;
	std::vector<Node*> nodes;
	nodes.reserve(m_data.m_packedNodes.size());

	// 1) Create every node and rebuild the detached scene hierarchy.
	// Packed nodes are parent-first, so every parent path resolves before its children are attached.
	for (size_t nodeIndex = 0; nodeIndex < m_data.m_packedNodes.size(); ++nodeIndex)
	{
		PackedNode const& packedNode = m_data.m_packedNodes[nodeIndex];
		Object*           object     = ClassDatabase::CreateInstance(packedNode.m_type);
		Node*             node       = dynamic_cast<Node*>(object);
		if (node == nullptr)
		{
			delete object;
			delete root;
			DebuggerPrintf("PackedScene: type '%s' is not a creatable Node.\n", packedNode.m_type.c_str());
			return nullptr;
		}

		node->SetName(packedNode.m_name);
		if (nodeIndex == 0)
		{
			if (!packedNode.m_parentPath.empty())
			{
				delete node;
				DebuggerPrintf("PackedScene: the first node must be the scene root and have no parent path.\n");
				return nullptr;
			}
			root = node;
		}
		else
		{
			if (packedNode.m_parentPath.empty())
			{
				delete node;
				delete root;
				DebuggerPrintf("PackedScene: node '%s' creates an additional scene root.\n", packedNode.m_name.c_str());
				return nullptr;
			}

			NodePath const parentPath(packedNode.m_parentPath);
			if (!parentPath.IsValid() || parentPath.IsAbsolute())
			{
				delete node;
				delete root;
				DebuggerPrintf(
					"PackedScene: node '%s' has invalid scene-relative parent path '%s'.\n",
					packedNode.m_name.c_str(),
					packedNode.m_parentPath.c_str());
				return nullptr;
			}

			Node* parent = root->GetNode(parentPath);
			if (parent == nullptr)
			{
				delete node;
				delete root;
				DebuggerPrintf(
					"PackedScene: node '%s' has unresolved parent path '%s'.\n",
					packedNode.m_name.c_str(),
					packedNode.m_parentPath.c_str());
				return nullptr;
			}

			std::string const requestedName = node->GetName();
			parent->AddNode(node);
			if (node->GetParent() != parent || node->GetName() != requestedName)
			{
				std::string const actualName = node->GetName();
				if (node->GetParent() == nullptr)
				{
					delete node;
				}
				delete root;
				DebuggerPrintf(
					"PackedScene: sibling name '%s' is not unique; AddNode produced '%s'.\n",
					requestedName.c_str(),
					actualName.c_str());
				return nullptr;
			}
		}
		nodes.push_back(node);
	}

	// 2) Apply properties after the complete hierarchy exists. Setters may safely
	// inspect parents or children, while SceneTree lifecycle callbacks have not started yet.
	for (size_t nodeIndex = 0; nodeIndex < m_data.m_packedNodes.size(); ++nodeIndex)
	{
		PackedNode const& packedNode = m_data.m_packedNodes[nodeIndex];
		Node*             node       = nodes[nodeIndex];
		for (PackedProperty const& packedProperty : packedNode.m_properties)
		{
			PropertyInfo const* property = ClassDatabase::FindProperty(packedNode.m_type, packedProperty.m_name);
			if (property == nullptr)
			{
				DebuggerPrintf(
					"PackedScene: skipping unknown property '%s' on type '%s'.\n",
					packedProperty.m_name.c_str(),
					packedNode.m_type.c_str());
				continue;
			}
			if (!packedProperty.CanApplyTo(*property))
			{
				DebuggerPrintf(
					"PackedScene: skipping incompatible property '%s' on type '%s'.\n",
					packedProperty.m_name.c_str(),
					packedNode.m_type.c_str());
				continue;
			}

			try
			{
				property->GetSetter()->Invoke(node, { packedProperty.m_value });
			}
			catch (std::exception const& error)
			{
				DebuggerPrintf(
					"PackedScene: failed to apply property '%s' on type '%s': %s\n",
					packedProperty.m_name.c_str(),
					packedNode.m_type.c_str(),
					error.what());
			}
		}
	}

	return root;
}

bool PackedScene::ParseNodeRecursively(
	Node const* node, std::string const& parentPath, std::vector<PackedNode>& outNodes)
{
	if (node == nullptr || !node->GetSerializable())
	{
		return true;
	}

	PackedNode packedNode;
	packedNode.m_name       = node->GetName();
	packedNode.m_type       = node->GetClassName();
	packedNode.m_parentPath = parentPath;
	if (packedNode.m_name.empty() || packedNode.m_name == "." || packedNode.m_name.find('/') != std::string::npos)
	{
		DebuggerPrintf(
			"PackedScene: node name '%s' cannot be represented in a scene path.\n",
			packedNode.m_name.c_str());
		return false;
	}

	Object* defaultObject = ClassDatabase::CreateInstance(packedNode.m_type);
	Node*   defaultNode   = dynamic_cast<Node*>(defaultObject);
	if (defaultObject != nullptr && defaultNode == nullptr)
	{
		DebuggerPrintf("PackedScene: default object for type '%s' is not a Node.\n", packedNode.m_type.c_str());
	}

	std::vector<PropertyInfo const*> properties = ClassDatabase::GetAllProperties(packedNode.m_type);
	for (PropertyInfo const* property : properties)
	{
		if (property == nullptr || !property->HasUsage(PropertyInfo::UsageFlags::Storage)
			|| property->GetGetter() == nullptr)
		{
			continue;
		}

		try
		{
			Variant value = property->GetGetter()->Invoke(const_cast<Node*>(node), {});
			if (defaultNode != nullptr)
			{
				Variant defaultValue = property->GetGetter()->Invoke(defaultNode, {});
				if (value == defaultValue)
				{
					continue;
				}
			}
			packedNode.m_properties.emplace_back(property->m_name, value);
		}
		catch (std::exception const& error)
		{
			DebuggerPrintf(
				"PackedScene: failed to read property '%s' on type '%s': %s\n",
				property->m_name.c_str(),
				packedNode.m_type.c_str(),
				error.what());
		}
	}

	delete defaultObject;
	outNodes.push_back(std::move(packedNode));

	std::string const nodePath =
		parentPath.empty() ? "." : (parentPath == "." ? node->GetName() : parentPath + "/" + node->GetName());
	for (Node const* child : node->GetChildren())
	{
		if (!ParseNodeRecursively(child, nodePath, outNodes))
		{
			return false;
		}
	}

	return true;
}

bool PackedScene::CopyFrom(Resource const& other)
{
	PackedScene const* otherScene = dynamic_cast<PackedScene const*>(&other);
	if (otherScene == nullptr)
	{
		return false;
	}

	m_data = otherScene->m_data;
	return true;
}
