#include "MingEngine/Scene/Import/JsonLoader.hpp"

#include "ThirdParty/nlohmann/json.hpp"

#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"

#include <fstream>

namespace
{
bool TryReadVec3(nlohmann::json const& values, Vec3& outValue)
{
	if (!values.is_array() || values.size() < 3)
	{
		return false;
	}

	outValue.x = values[0].get<float>();
	outValue.y = values[1].get<float>();
	outValue.z = values[2].get<float>();
	return true;
}

bool TryReadVec3Array(nlohmann::json const& root, char const* key, std::vector<Vec3>& outValues)
{
	auto const found = root.find(key);
	if (found == root.end() || !found->contains("buffer") || !(*found)["buffer"].is_array())
	{
		return false;
	}

	nlohmann::json const& buffer = (*found)["buffer"];
	outValues.clear();
	outValues.reserve(buffer.size());
	for (nlohmann::json const& item : buffer)
	{
		Vec3 value;
		if (!TryReadVec3(item, value))
		{
			outValues.clear();
			return false;
		}

		outValues.push_back(value);
	}

	return true;
}

bool TryReadIndices(nlohmann::json const& root, std::vector<unsigned int>& outIndices)
{
	auto const found = root.find("indices");
	if (found == root.end() || !found->contains("buffer") || !(*found)["buffer"].is_array())
	{
		return false;
	}

	nlohmann::json const& buffer = (*found)["buffer"];
	outIndices.clear();
	outIndices.reserve(buffer.size());
	for (nlohmann::json const& item : buffer)
	{
		if (!item.is_number_unsigned() && !item.is_number_integer())
		{
			outIndices.clear();
			return false;
		}

		int const index = item.get<int>();
		if (index < 0)
		{
			outIndices.clear();
			return false;
		}

		outIndices.push_back(static_cast<unsigned int>(index));
	}

	return true;
}
} // namespace

bool JsonLoader::LoadFromFile(char const* filePath, MeshData& outMesh, JsonLoadOptions const& options)
{
	outMesh.Clear();
	if (filePath == nullptr || filePath[0] == '\0')
	{
		return false;
	}

	std::ifstream input(filePath);
	if (!input.is_open())
	{
		return false;
	}

	nlohmann::json root;
	try
	{
		input >> root;
	}
	catch (...)
	{
		return false;
	}

	std::vector<Vec3> positions;
	std::vector<Vec3> normals;
	std::vector<unsigned int> indices;
	try
	{
		if (!TryReadVec3Array(root, "Vertex_Position", positions) ||
			!TryReadVec3Array(root, "Vertex_Normal", normals) ||
			!TryReadIndices(root, indices))
		{
			return false;
		}
	}
	catch (...)
	{
		return false;
	}

	if (positions.empty() || positions.size() != normals.size() || indices.empty() || (indices.size() % 3) != 0)
	{
		return false;
	}

	outMesh.m_vertices.reserve(positions.size());
	for (size_t vertexIndex = 0; vertexIndex < positions.size(); ++vertexIndex)
	{
		outMesh.m_vertices.emplace_back(
			positions[vertexIndex] * options.m_uniformScale,
			options.m_defaultColor,
			Vec2::Zero,
			Vec3::Zero,
			Vec3::Zero,
			normals[vertexIndex]
		);
	}

	outMesh.m_indices.reserve(indices.size());
	for (size_t index = 0; index + 2 < indices.size(); index += 3)
	{
		unsigned int const i0 = indices[index];
		unsigned int       i1 = indices[index + 1];
		unsigned int       i2 = indices[index + 2];
		if (i0 >= outMesh.m_vertices.size() || i1 >= outMesh.m_vertices.size() || i2 >= outMesh.m_vertices.size())
		{
			outMesh.Clear();
			return false;
		}

		if (options.m_flipWindingOrder)
		{
			unsigned int const temp = i1;
			i1                      = i2;
			i2                      = temp;
		}

		outMesh.m_indices.push_back(i0);
		outMesh.m_indices.push_back(i1);
		outMesh.m_indices.push_back(i2);
	}

	return !outMesh.IsEmpty();
}

