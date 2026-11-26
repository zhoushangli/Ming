#include "MingEngine/Core/IO/ResourceFormatText.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/VariantJson.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef GetClassName
#include <fstream>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

namespace
{
using Json = VariantJson::Json;

bool IsTextResource(std::string const& type)
{
	for (char const* specialized : { "PackedScene", "ProjectSettings", "TextureResource", "MeshResource", "ShaderResource", "FontResource", "CSharpScript", "Theme", "ThemeStyleBox" })
	{
		if (ClassDatabase::IsSubclassOf(type, specialized))
		{
			return false;
		}
	}

	return ClassDatabase::IsSubclassOf(type, "Resource");
}

std::string Trim(std::string const& text)
{
	auto begin = text.find_first_not_of(" \t\r\n");
	return begin == std::string::npos ? std::string() : text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

std::string Quote(std::string const& text)
{
	return Json(text).dump();
}

char const* Constructor(Variant::Type type)
{
	switch (type)
	{
	case Variant::Type::Vector2: return "Vector2";
	case Variant::Type::Vector3: return "Vector3";
	case Variant::Type::Vector4: return "Vector4";
	case Variant::Type::Color: return "Color";
	case Variant::Type::EulerAngles: return "EulerAngles";
	case Variant::Type::Matrix4x4: return "Matrix4x4";
	case Variant::Type::AABB2: return "AABB2";
	case Variant::Type::OBB2: return "OBB2";
	case Variant::Type::Capsule3: return "Capsule3";
	case Variant::Type::UInt32: return "UInt32";
	case Variant::Type::Int64: return "Int64";
	default: return "";
	}
}

std::string Argument(std::string const& text, std::string const& constructor)
{
	std::string const prefix = constructor + "(";
	if (text.rfind(prefix, 0) != 0 || text.back() != ')')
	{
		throw std::runtime_error("Expected " + constructor + "(...)");
	}

	return text.substr(prefix.size(), text.size() - prefix.size() - 1);
}

std::map<std::string, std::string> Header(std::string const& line, std::string& kind)
{
	if (line.size() < 2 || line.front() != '[' || line.back() != ']')
	{
		throw std::runtime_error("Expected section header");
	}

	std::string const body = line.substr(1, line.size() - 2);
	size_t cursor = body.find(' ');
	kind = body.substr(0, cursor);
	std::map<std::string, std::string> fields;
	while (cursor != std::string::npos && cursor < body.size())
	{
		cursor = body.find_first_not_of(" \t", cursor);
		if (cursor == std::string::npos)
		{
			break;
		}

		size_t const equal = body.find('=', cursor);
		if (equal == std::string::npos)
		{
			throw std::runtime_error("Expected header assignment");
		}

		std::string const name = body.substr(cursor, equal - cursor);
		size_t end = equal + 1;
		bool quoted = false;
		bool escaped = false;
		for (; end < body.size(); ++end)
		{
			char c = body[end];
			if (escaped)
			{
				escaped = false;
			}
			else if (c == '\\' && quoted)
			{
				escaped = true;
			}
			else if (c == '"')
			{
				quoted = !quoted;
			}
			else if (!quoted && (c == ' ' || c == '\t'))
			{
				break;
			}
		}

		Json value = Json::parse(body.substr(equal + 1, end - equal - 1));
		if (!fields.emplace(name, value.is_string() ? value.get<std::string>() : value.dump()).second)
		{
			throw std::runtime_error("Duplicate header field " + name);
		}

		cursor = end;
	}

	return fields;
}

Variant ReadValue(std::string const& text, PropertyInfo const& property, std::map<std::string, Ref<Resource>> const& external)
{
	if (property.m_type == Variant::Type::ObjectPtr)
	{
		if (text == "null")
		{
			return Variant(static_cast<Object*>(nullptr));
		}

		std::string const id = Json::parse(Argument(text, "ExtResource")).get<std::string>();
		auto found = external.find(id);
		if (found == external.end() || !ClassDatabase::IsSubclassOf(found->second->GetClassName(), property.m_hintData))
		{
			throw std::runtime_error("Missing or incompatible external resource " + id);
		}

		return Variant(found->second);
	}

	std::string const constructor = Constructor(property.m_type);
	Json json;
	if (!constructor.empty())
	{
		std::string const arguments = Argument(text, constructor);
		bool const integer = property.m_type == Variant::Type::UInt32 || property.m_type == Variant::Type::Int64;
		json = Json::parse(integer ? arguments : "[" + arguments + "]");
	}
	else
	{
		json = Json::parse(text);
		if (json.is_array() || json.is_object())
		{
			throw std::runtime_error("Containers are not supported");
		}
	}

	if (property.m_type == Variant::Type::Color)
	{
		for (auto const& channel : json)
		{
			if (!channel.is_number_integer() || channel.get<int>() < 0 || channel.get<int>() > 255)
			{
				throw std::runtime_error("Color channels must be integers from 0 to 255");
			}
		}
	}

	Variant value;
	if (!VariantJson::TryDeserialize(json, property.m_type, value))
	{
		throw std::runtime_error("Invalid value for property " + property.m_name);
	}

	return value;
}

struct TextWriter
{
	std::map<std::string, std::string> m_ids;
	std::string m_external;

	std::string Value(Variant const& value)
	{
		if (value.GetType() == Variant::Type::ObjectPtr)
		{
			Object* object = value.As<Object*>();
			if (object == nullptr)
			{
				return "null";
			}

			auto* resource = dynamic_cast<Resource*>(object);
			if (resource == nullptr)
			{
				throw std::runtime_error("Only external Resource references can be saved");
			}

			VirtualPath const& path = resource->GetSourceFilePath().IsValid() ? resource->GetSourceFilePath() : resource->GetVirtualPath();
			if (!path.IsValid())
			{
				throw std::runtime_error("Referenced resource has no persistent path");
			}

			auto found = m_ids.find(path.GetString());
			if (found == m_ids.end())
			{
				std::string id = std::to_string(m_ids.size() + 1);
				found = m_ids.emplace(path.GetString(), id).first;
				m_external += "[ext_resource type=" + Quote(resource->GetClassName()) + " path=" + Quote(path.GetString()) + " id=" + Quote(id) + "]\n";
			}

			return "ExtResource(" + Quote(found->second) + ")";
		}

		Json json;
		if (!VariantJson::TrySerialize(value, json))
		{
			throw std::runtime_error("Unsupported Variant type");
		}

		if (json.is_number_float() && !std::isfinite(json.get<double>()))
		{
			throw std::runtime_error("Nonfinite numbers cannot be saved");
		}

		if (json.is_array())
		{
			for (auto const& component : json)
			{
				if (!component.is_number() || !std::isfinite(component.get<double>()))
				{
					throw std::runtime_error("Invalid mathematical component");
				}
			}
		}

		std::string result = json.dump();
		std::string const constructor = Constructor(value.GetType());
		if (!constructor.empty())
		{
			if (json.is_array())
			{
				result = result.substr(1, result.size() - 2);
			}

			return constructor + "(" + result + ")";
		}

		return result;
	}
};
}

std::vector<std::string> ResourceFormatLoaderText::GetSupportedExtensions() const
{
	return { ".tres", ".tscn" };
}

Ref<Resource> ResourceFormatLoaderText::Load(VirtualPath const& path)
{
	std::string text;
	if (g_engine == nullptr || !g_engine->m_fileSystem->ReadText(path, text))
	{
		return {};
	}

	std::string error;
	Ref<Resource> resource = Parse(text, path, error);
	if (!resource.IsValid())
	{
		ERR_PRINT(error);
	}

	return resource;
}

Ref<Resource> ResourceFormatLoaderText::Parse(std::string const& text, VirtualPath const& path, std::string& error)
{
	size_t lineNumber = 0;
	try
	{
		std::istringstream stream(text);
		std::string line;
		std::string kind;
		Ref<Resource> resource;
		Ref<PackedScene> scene;
		PackedSceneData data;
		std::map<std::string, Ref<Resource>> external;
		std::vector<std::pair<PropertyInfo const*, Variant>> values;
		std::set<std::string> assigned;
		bool body = false;
		while (std::getline(stream, line))
		{
			++lineNumber;
			line = Trim(line);
			if (line.empty() || line.front() == ';' || line.front() == '#')
			{
				continue;
			}

			if (line.front() == '[')
			{
				auto fields = Header(line, kind);
				if (!resource.IsValid())
				{
					if (fields.at("format") != "1" || (kind != "ming_resource" && kind != "ming_scene"))
					{
						throw std::runtime_error("Unsupported text resource header or version");
					}

					if (kind == "ming_scene" && path.HasExtension(".tscn"))
					{
						scene = CreateRef<PackedScene>();
						resource = scene;
					}
					else if (kind == "ming_resource" && path.HasExtension(".tres"))
					{
						std::string const type = fields.at("type");
						if (!IsTextResource(type))
						{
							throw std::runtime_error("Invalid resource class " + type);
						}

						resource = Ref<Resource>(dynamic_cast<Resource*>(ClassDatabase::CreateInstance(type)));
					}
					if (!resource.IsValid())
					{
						throw std::runtime_error("Could not create resource or file extension does not match header");
					}
				}
				else if (kind == "ext_resource" && !body)
				{
					Ref<Resource> dependency = ResourceLoader::Load(VirtualPath(fields.at("path")));
					if (!dependency.IsValid() || !ClassDatabase::IsSubclassOf(dependency->GetClassName(), fields.at("type"))
						|| !external.emplace(fields.at("id"), dependency).second)
					{
						throw std::runtime_error("Missing, duplicate or incompatible dependency");
					}
				}
				else if (kind == "node" && scene.IsValid())
				{
					PackedNode node;
					node.m_name = fields.at("name");
					node.m_type = fields.at("type");
					node.m_parentPath = fields.at("parent");
					if (!ClassDatabase::IsSubclassOf(node.m_type.ToUtf8(), "Node") || node.m_name.IsEmpty()
						|| node.m_name == "." || node.m_name.Contains(U'/'))
					{
						throw std::runtime_error("Invalid scene node");
					}

					data.m_packedNodes.push_back(std::move(node));
					assigned.clear();
					body = true;
				}
				else if (kind == "resource" && !scene.IsValid() && !body)
				{
					body = true;
				}
				else
				{
					throw std::runtime_error("Unexpected section " + kind);
				}

				continue;
			}

			size_t const equal = line.find('=');
			if (!body || equal == std::string::npos)
			{
				throw std::runtime_error("Expected property assignment");
			}

			std::string const name = Trim(line.substr(0, equal));
			std::string const type = scene.IsValid() ? data.m_packedNodes.back().m_type.ToUtf8() : resource->GetClassName();
			PropertyInfo const* property = ClassDatabase::FindProperty(type, name);
			if (property == nullptr || !property->HasUsage(PropertyInfo::UsageFlags::Storage)
				|| property->GetSetter() == nullptr || !assigned.insert(name).second)
			{
				throw std::runtime_error("Unknown, duplicate or unwritable property " + type + "." + name);
			}

			Variant value = ReadValue(Trim(line.substr(equal + 1)), *property, external);
			if (scene.IsValid())
			{
				data.m_packedNodes.back().m_properties.emplace_back(name, value);
			}
			else
			{
				values.emplace_back(property, value);
			}
		}

		if (!body)
		{
			throw std::runtime_error("Missing resource body");
		}

		if (scene.IsValid())
		{
			std::set<std::string> paths;
			for (size_t i = 0; i < data.m_packedNodes.size(); ++i)
			{
				auto const& node = data.m_packedNodes[i];
				std::string parent = node.m_parentPath.ToUtf8();
				if ((i == 0) != parent.empty())
				{
					throw std::runtime_error("Scene must have exactly one root, listed first");
				}

				std::string const nodePath = i == 0 ? "." : parent == "." ? node.m_name.ToUtf8() : parent + "/" + node.m_name.ToUtf8();
				if (i != 0 && paths.count(parent) == 0)
				{
					throw std::runtime_error("Scene parents must precede their children");
				}

				if (!paths.insert(nodePath).second)
				{
					throw std::runtime_error("Duplicate node path " + nodePath);
				}
			}

			for (size_t i = 1; i < data.m_packedNodes.size(); ++i)
			{
				if (paths.count(data.m_packedNodes[i].m_parentPath.ToUtf8()) == 0)
				{
					throw std::runtime_error("Missing scene parent");
				}
			}

			scene->m_data = std::move(data);
		}
		else
		{
			for (auto const& entry : values)
			{
				entry.first->GetSetter()->Invoke(resource.Get(), { entry.second });
				if (entry.first->GetGetter() != nullptr && entry.first->GetGetter()->Invoke(resource.Get(), {}) != entry.second)
				{
					throw std::runtime_error("Setter rejected property " + entry.first->m_name);
				}
			}
		}

		resource->SetVirtualPath(path);
		return resource;
	}
	catch (std::exception const& exception)
	{
		error = path.GetString() + ":" + std::to_string(lineNumber) + ": " + exception.what();
		return {};
	}
}

bool ResourceFormatSaverText::CanSave(VirtualPath const& path, Variant const& value) const
{
	Ref<Resource> resource(value);
	return resource.IsValid() && (dynamic_cast<PackedScene*>(resource.Get()) != nullptr ? path.HasExtension(".tscn") : path.HasExtension(".tres") && IsTextResource(resource->GetClassName()));
}

bool ResourceFormatSaverText::Write(Ref<Resource> const& resource, std::string& text, std::string& error)
{
	try
	{
		if (!resource.IsValid())
		{
			throw std::runtime_error("Cannot save a null resource");
		}

		TextWriter writer;
		std::string body;
		auto* scene = dynamic_cast<PackedScene*>(resource.Get());
		if (scene == nullptr && !IsTextResource(resource->GetClassName()))
		{
			throw std::runtime_error("This resource requires its specialized file format");
		}

		if (scene != nullptr)
		{
			if (scene->m_data.m_packedNodes.empty())
			{
				throw std::runtime_error("Cannot save an empty PackedScene");
			}
			for (auto const& node : scene->m_data.m_packedNodes)
			{
				body += "\n[node name=" + Quote(node.m_name.ToUtf8()) + " type=" + Quote(node.m_type.ToUtf8()) + " parent=" + Quote(node.m_parentPath.ToUtf8()) + "]\n";
				for (auto const& property : node.m_properties)
				{
					body += property.m_name.ToUtf8() + " = " + writer.Value(property.m_value) + "\n";
				}
			}
		}
		else
		{
			body = "\n[resource]\n";
			auto properties = ClassDatabase::GetAllProperties(resource->GetClassName());
			std::sort(properties.begin(), properties.end(), [](auto* a, auto* b) { return a->m_name < b->m_name; });
			for (auto const* property : properties)
			{
				if (!property->HasUsage(PropertyInfo::UsageFlags::Storage))
				{
					continue;
				}

				if (property->GetGetter() == nullptr || property->GetSetter() == nullptr)
				{
					throw std::runtime_error("Stored property has no getter or setter: " + property->m_name);
				}

				body += property->m_name + " = " + writer.Value(property->GetGetter()->Invoke(resource.Get(), {})) + "\n";
			}
		}

		text = scene != nullptr ? "[ming_scene format=1]\n" : "[ming_resource type=" + Quote(resource->GetClassName()) + " format=1]\n";
		text += "\n" + writer.m_external + body;
		return true;
	}
	catch (std::exception const& exception)
	{
		error = exception.what();
		return false;
	}
}

bool ResourceFormatSaverText::Save(VirtualPath const& path, Variant const& value)
{
	Ref<Resource> resource(value);
	std::string text;
	std::string error;
	if (!CanSave(path, value) || !Write(resource, text, error))
	{
		ERR_PRINT("Could not save " + path.GetString() + ": " + error);
		return false;
	}

	std::filesystem::path physical;
	if (g_engine == nullptr || !g_engine->m_fileSystem->TryGetPhysicalPath(path, physical))
	{
		return false;
	}

	static std::atomic<unsigned int> serial{0};
	std::filesystem::path temporary = physical;
	temporary += ".saving." + std::to_string(GetCurrentProcessId()) + "." + std::to_string(++serial);
	std::error_code fileError;
	std::filesystem::create_directories(physical.parent_path(), fileError);
	if (fileError)
	{
		return false;
	}

	std::ofstream output(temporary, std::ios::binary);
	output << text;
	output.close();
	bool const saved = output.good() && MoveFileExW(temporary.c_str(), physical.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
	if (!saved)
	{
		std::filesystem::remove(temporary, fileError);
		return false;
	}

	resource->SetVirtualPath(path);
	return true;
}
