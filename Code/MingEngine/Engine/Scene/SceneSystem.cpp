#include "MingEngine/Engine/Scene/SceneSystem.hpp"

#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Script/CSharpScript.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

SceneSystem::~SceneSystem()
{
	Shutdown();
}

void SceneSystem::Startup()
{
	if (m_sceneTree == nullptr)
	{
		m_sceneTree = MemNew<SceneTree>();
	}
}

void SceneSystem::Shutdown()
{
	SceneTree* tree = m_sceneTree;
	m_sceneTree = nullptr;
	MemDelete(tree);
	m_scenePath = VirtualPath();
	m_pathSceneID = ObjectID::Invalid;
	m_autoloads.clear();
}

void SceneSystem::Reset()
{
	Shutdown();
	Startup();
}

Node* SceneSystem::GetCurrentScene() const
{
	return m_sceneTree != nullptr ? m_sceneTree->GetScene() : nullptr;
}

bool SceneSystem::ChangeScene(VirtualPath const& path, std::string& error)
{
	error.clear();
	if (m_sceneTree == nullptr || !path.IsValid())
	{
		error = "Cannot load scene: no scene tree or invalid scene path.";
		return false;
	}

	Ref<PackedScene> packed(ResourceLoader::LoadUncached(path));
	Node* scene = packed.IsValid() ? packed->Instantiate() : nullptr;
	if (scene == nullptr)
	{
		error = "Cannot instantiate scene: " + path.GetString();
		return false;
	}

	if (!ChangeScene(scene, error))
	{
		MemDelete(scene);
		return false;
	}

	m_scenePath = path;
	m_pathSceneID = scene->GetObjectID();
	return true;
}

bool SceneSystem::ChangeScene(Node* scene, std::string& error)
{
	error.clear();
	if (m_sceneTree == nullptr || scene == nullptr || scene->GetParent() != nullptr || scene->GetSceneTree() != nullptr)
	{
		error = "ChangeScene requires a detached scene root and a running scene tree.";
		return false;
	}

	m_sceneTree->ChangeScene(scene);
	m_scenePath = VirtualPath();
	m_pathSceneID = ObjectID::Invalid;
	return true;
}

bool SceneSystem::ReloadScene(std::string& error)
{
	Node* scene = GetCurrentScene();
	if (!m_scenePath.IsValid() || scene == nullptr || scene->GetObjectID() != m_pathSceneID || scene->IsQueuedForDeletion())
	{
		error = "The current scene has no reloadable scene path.";
		return false;
	}

	VirtualPath const path = m_scenePath;
	return ChangeScene(path, error);
}

void SceneSystem::ClearScene()
{
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->ClearScene();
	}

	m_scenePath = VirtualPath();
	m_pathSceneID = ObjectID::Invalid;
}

bool SceneSystem::CreateAutoloads(std::vector<ProjectSettings::AutoLoad> const& entries, std::string& error)
{
	error.clear();
	if (m_sceneTree == nullptr || !m_autoloads.empty())
	{
		error = "Autoloads require a running scene tree with no existing autoloads.";
		return false;
	}

	std::vector<Node*> autoloads;
	for (ProjectSettings::AutoLoad const& entry : entries)
	{
		Node* node = nullptr;
		if (entry.m_path.HasExtension(".tscn"))
		{
			Ref<PackedScene> scene(ResourceLoader::LoadUncached(entry.m_path));
			node = scene.IsValid() ? scene->Instantiate() : nullptr;
		}
		else
		{
			Ref<CSharpScript> script(ResourceLoader::LoadUncached(entry.m_path));
			std::string const base = g_engine->m_scriptSystem->GetScriptNativeBase(entry.m_path);
			if (script.IsValid() && ClassDatabase::IsSubclassOf(base, "Node"))
			{
				node = dynamic_cast<Node*>(ClassDatabase::CreateInstance(base));
				if (node != nullptr)
				{
					node->SetScript(Variant(script.Get()));
					if (node->GetScriptInstance() == nullptr)
					{
						MemDelete(node);
						node = nullptr;
					}
				}
			}
		}

		if (node == nullptr || !g_engine->m_scriptSystem->ValidateSceneScripts(node))
		{
			MemDelete(node);
			for (Node* pending : autoloads)
			{
				MemDelete(pending);
			}

			error = "Cannot load autoload " + entry.m_name + ": " + entry.m_path.GetString();
			return false;
		}

		node->SetName(entry.m_name.c_str());
		autoloads.push_back(node);
	}

	for (Node* node : autoloads)
	{
		m_autoloads.push_back(node->GetObjectID());
		m_sceneTree->GetRoot()->AddNode(node);
	}

	return true;
}

void SceneSystem::ClearAutoloads()
{
	for (ObjectID id : m_autoloads)
	{
		Node* node = ObjectDatabase::GetInstance<Node>(id);
		if (node != nullptr && node->GetSceneTree() == m_sceneTree)
		{
			node->QueueFree();
		}
	}

	m_autoloads.clear();
}

void SceneSystem::Process(float deltaSeconds)
{
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->Process(deltaSeconds);
	}
}

void SceneSystem::PhysicsProcess(float fixedDeltaSeconds)
{
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->PhysicsProcess(fixedDeltaSeconds);
	}
}

void SceneSystem::EndFrame()
{
	if (m_sceneTree != nullptr)
	{
		m_sceneTree->FlushPendingNode();
	}
}
