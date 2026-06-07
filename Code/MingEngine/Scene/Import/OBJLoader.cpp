#include "MingEngine/Scene/Import/OBJLoader.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Math/Vec2.hpp"
#include "MingEngine/Engine/Math/Vec3.hpp"
#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

namespace
{
std::string TrimWhitespace(std::string const& text)
{
	size_t start = text.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
	{
		return "";
	}

	size_t end = text.find_last_not_of(" \t\r\n");
	return text.substr(start, end - start + 1);
}

bool StartsWithKeyword(std::string const& line, char const* keyword)
{
	size_t const keywordLength = strlen(keyword);
	if (line.size() < keywordLength)
	{
		return false;
	}

	if (line.compare(0, keywordLength, keyword) != 0)
	{
		return false;
	}

	return line.size() == keywordLength || line[keywordLength] == ' ' || line[keywordLength] == '\t';
}

std::string GetDirectiveValue(std::string const& line, char const* keyword)
{
	if (!StartsWithKeyword(line, keyword))
	{
		return "";
	}

	size_t const keywordLength = strlen(keyword);
	return TrimWhitespace(line.substr(keywordLength));
}

std::vector<std::string> Tokenize(std::string const& line)
{
	std::vector<std::string> tokens;
	std::istringstream       stream(line);
	std::string              token;
	while (stream >> token)
	{
		tokens.push_back(token);
	}

	return tokens;
}

std::string GetDirectoryPath(std::string const& filePath)
{
	size_t const slashIndex = filePath.find_last_of("/\\");
	if (slashIndex == std::string::npos)
	{
		return "";
	}

	return filePath.substr(0, slashIndex + 1);
}

bool IsAbsolutePath(std::string const& filePath)
{
	if (filePath.size() >= 2 && filePath[1] == ':')
	{
		return true;
	}

	return !filePath.empty() && (filePath[0] == '/' || filePath[0] == '\\');
}

std::string JoinPath(std::string const& directory, std::string const& filePath)
{
	if (filePath.empty() || IsAbsolutePath(filePath) || directory.empty())
	{
		return filePath;
	}

	char const lastChar = directory.back();
	if (lastChar == '/' || lastChar == '\\')
	{
		return directory + filePath;
	}

	return directory + "/" + filePath;
}

struct OBJVertexKey
{
	int m_positionIndex = -1;
	int m_texCoordIndex = -1;
	int m_normalIndex   = -1;

	bool operator<(OBJVertexKey const& other) const
	{
		if (m_positionIndex != other.m_positionIndex)
		{
			return m_positionIndex < other.m_positionIndex;
		}

		if (m_texCoordIndex != other.m_texCoordIndex)
		{
			return m_texCoordIndex < other.m_texCoordIndex;
		}

		return m_normalIndex < other.m_normalIndex;
	}
};

bool TryParseFloat3(std::vector<std::string> const& tokens, Vec3& outValue)
{
	if (tokens.size() < 4)
	{
		return false;
	}

	outValue.x = static_cast<float>(atof(tokens[1].c_str()));
	outValue.y = static_cast<float>(atof(tokens[2].c_str()));
	outValue.z = static_cast<float>(atof(tokens[3].c_str()));
	return true;
}

bool TryParseFloat2(std::vector<std::string> const& tokens, Vec2& outValue)
{
	if (tokens.size() < 3)
	{
		return false;
	}

	outValue.x = static_cast<float>(atof(tokens[1].c_str()));
	outValue.y = static_cast<float>(atof(tokens[2].c_str()));
	return true;
}

int ResolveOBJIndex(int rawIndex, int count)
{
	if (rawIndex > 0)
	{
		return rawIndex - 1;
	}

	if (rawIndex < 0)
	{
		return count + rawIndex;
	}

	return -1;
}

bool TryParseFaceVertex(
	std::string const& token, int positionCount, int texCoordCount, int normalCount, OBJVertexKey& outKey
)
{
	outKey = OBJVertexKey();

	size_t firstSlash  = token.find('/');
	size_t secondSlash = firstSlash == std::string::npos ? std::string::npos : token.find('/', firstSlash + 1);

	std::string positionText = firstSlash == std::string::npos ? token : token.substr(0, firstSlash);
	std::string texCoordText;
	std::string normalText;

	if (firstSlash != std::string::npos)
	{
		if (secondSlash == std::string::npos)
		{
			texCoordText = token.substr(firstSlash + 1);
		}
		else
		{
			texCoordText = token.substr(firstSlash + 1, secondSlash - firstSlash - 1);
			normalText   = token.substr(secondSlash + 1);
		}
	}

	if (positionText.empty())
	{
		return false;
	}

	outKey.m_positionIndex = ResolveOBJIndex(atoi(positionText.c_str()), positionCount);
	if (outKey.m_positionIndex < 0 || outKey.m_positionIndex >= positionCount)
	{
		return false;
	}

	if (!texCoordText.empty())
	{
		outKey.m_texCoordIndex = ResolveOBJIndex(atoi(texCoordText.c_str()), texCoordCount);
		if (outKey.m_texCoordIndex < 0 || outKey.m_texCoordIndex >= texCoordCount)
		{
			return false;
		}
	}

	if (!normalText.empty())
	{
		outKey.m_normalIndex = ResolveOBJIndex(atoi(normalText.c_str()), normalCount);
		if (outKey.m_normalIndex < 0 || outKey.m_normalIndex >= normalCount)
		{
			return false;
		}
	}

	return true;
}

unsigned int GetOrCreateVertex(
	OBJVertexKey const&                   key,
	std::vector<Vec3> const&              positions,
	std::vector<Vec2> const&              texCoords,
	std::vector<Vec3> const&              normals,
	OBJLoadOptions const&                 options,
	std::vector<Vertex>&                  outVertices,
	std::map<OBJVertexKey, unsigned int>& vertexLookup
)
{
	auto const found = vertexLookup.find(key);
	if (found != vertexLookup.end())
	{
		return found->second;
	}

	Vec2 uv = Vec2::Zero;
	if (key.m_texCoordIndex >= 0)
	{
		uv = texCoords[key.m_texCoordIndex];
		if (options.m_invertVTexCoord)
		{
			uv.y = 1.f - uv.y;
		}
	}

	Vec3 normal = Vec3::Zero;
	if (key.m_normalIndex >= 0)
	{
		normal = normals[key.m_normalIndex];
	}

	unsigned int const vertexIndex = static_cast<unsigned int>(outVertices.size());
	outVertices.emplace_back(
		Vertex(positions[key.m_positionIndex], options.m_defaultColor, uv, Vec3::Zero, Vec3::Zero, normal)
	);
	vertexLookup[key] = vertexIndex;
	return vertexIndex;
}

struct OBJMaterialTexturePaths
{
	std::string m_diffuseTexturePath;
};

OBJMaterialTexturePaths FindTexturePathsInMTL(std::string const& mtlPath, std::string const& materialName)
{
	std::ifstream input(mtlPath);
	if (!input.is_open())
	{
		return OBJMaterialTexturePaths();
	}

	std::string const       mtlDirectory = GetDirectoryPath(mtlPath);
	std::string             activeMaterialName;
	OBJMaterialTexturePaths selectedTexturePaths;
	OBJMaterialTexturePaths fallbackTexturePaths;
	std::string             line;
	while (std::getline(input, line))
	{
		std::string const trimmed = TrimWhitespace(line);
		if (trimmed.empty() || trimmed[0] == '#')
		{
			continue;
		}

		std::string const newMaterialName = GetDirectiveValue(trimmed, "newmtl");
		if (!newMaterialName.empty())
		{
			activeMaterialName = newMaterialName;
			continue;
		}

		std::string const diffuseTexturePath = GetDirectiveValue(trimmed, "map_Kd");
		if (diffuseTexturePath.empty())
		{
			continue;
		}

		OBJMaterialTexturePaths resolvedTexturePaths;
		resolvedTexturePaths.m_diffuseTexturePath = JoinPath(mtlDirectory, diffuseTexturePath);

		if (!materialName.empty() && activeMaterialName == materialName)
		{
			if (!resolvedTexturePaths.m_diffuseTexturePath.empty())
			{
				selectedTexturePaths.m_diffuseTexturePath = resolvedTexturePaths.m_diffuseTexturePath;
			}
		}

		if (fallbackTexturePaths.m_diffuseTexturePath.empty())
		{
			fallbackTexturePaths.m_diffuseTexturePath = resolvedTexturePaths.m_diffuseTexturePath;
		}
	}

	if (!selectedTexturePaths.m_diffuseTexturePath.empty())
	{
		return selectedTexturePaths;
	}

	return fallbackTexturePaths;
}

void TryLoadOBJTextures(
	std::string const& objPath,
	std::string const& materialLibraryPath,
	std::string const& materialName,
	Texture*&          outDiffuseTexture
)
{
	outDiffuseTexture = nullptr;

	if (materialLibraryPath.empty() || g_engine == nullptr || g_engine->m_renderer == nullptr)
	{
		return;
	}

	std::string const             objDirectory    = GetDirectoryPath(objPath);
	std::string const             resolvedMTLPath = JoinPath(objDirectory, materialLibraryPath);
	OBJMaterialTexturePaths const texturePaths    = FindTexturePathsInMTL(resolvedMTLPath, materialName);
	if (!texturePaths.m_diffuseTexturePath.empty())
	{
		outDiffuseTexture = g_engine->m_renderer->CreateOrGetTexture(texturePaths.m_diffuseTexturePath.c_str());
	}
}
} // namespace

bool OBJLoader::LoadFromFile(
	char const* filePath, MeshData& outMesh, OBJHeader& outHeader, OBJLoadOptions const& options
)
{
	outMesh.Clear();
	outHeader = OBJHeader();

	if (filePath == nullptr || filePath[0] == '\0')
	{
		outHeader.m_errorMessage = "OBJLoader::LoadFromFile requires a valid file path";
		return false;
	}

	std::ifstream input(filePath);
	GUARANTEE_OR_DIE(input.good(), Stringf("Failed to open OBJ file: %s", filePath));
	if (!input.is_open())
	{
		outHeader.m_errorMessage = "Failed to open OBJ file";
		return false;
	}

	std::vector<Vec3>                    positions;
	std::vector<Vec2>                    texCoords;
	std::vector<Vec3>                    normals;
	std::map<OBJVertexKey, unsigned int> vertexLookup;
	bool                                 allFaceVerticesHaveNormals = true;
	std::string                          materialLibraryPath;
	std::string                          materialName;

	std::string line;
	while (std::getline(input, line))
	{
		std::string const trimmed = TrimWhitespace(line);
		if (trimmed.empty() || trimmed[0] == '#')
		{
			continue;
		}

		std::vector<std::string> const tokens = Tokenize(trimmed);
		if (tokens.empty())
		{
			continue;
		}

		std::string const parsedMaterialLibraryPath = GetDirectiveValue(trimmed, "mtllib");
		if (!parsedMaterialLibraryPath.empty())
		{
			materialLibraryPath = parsedMaterialLibraryPath;
			continue;
		}

		std::string const parsedMaterialName = GetDirectiveValue(trimmed, "usemtl");
		if (!parsedMaterialName.empty())
		{
			materialName = parsedMaterialName;
			continue;
		}

		if (tokens[0] == "v")
		{
			Vec3 position;
			if (!TryParseFloat3(tokens, position))
			{
				outMesh.Clear();
				outHeader.m_errorMessage = "Malformed OBJ vertex position";
				return false;
			}

			positions.push_back(position);
			continue;
		}

		if (tokens[0] == "vt")
		{
			Vec2 texCoord;
			if (!TryParseFloat2(tokens, texCoord))
			{
				outMesh.Clear();
				outHeader.m_errorMessage = "Malformed OBJ texture coordinate";
				return false;
			}

			texCoords.push_back(texCoord);
			continue;
		}

		if (tokens[0] == "vn")
		{
			Vec3 normal;
			if (!TryParseFloat3(tokens, normal))
			{
				outMesh.Clear();
				outHeader.m_errorMessage = "Malformed OBJ normal";
				return false;
			}

			normals.push_back(normal);
			continue;
		}

		if (tokens[0] == "f")
		{
			if (tokens.size() < 4)
			{
				outMesh.Clear();
				outHeader.m_errorMessage = "OBJ face must have at least three vertices";
				return false;
			}

			int const faceVertexCount = static_cast<int>(tokens.size()) - 1;
			if (!options.m_triangulateFaces && faceVertexCount != 3)
			{
				outMesh.Clear();
				outHeader.m_errorMessage = "OBJ face is not a triangle";
				return false;
			}

			std::vector<unsigned int> faceIndices;
			faceIndices.reserve(faceVertexCount);

			for (int tokenIndex = 1; tokenIndex < static_cast<int>(tokens.size()); ++tokenIndex)
			{
				OBJVertexKey key;
				if (!TryParseFaceVertex(
						tokens[tokenIndex],
						static_cast<int>(positions.size()),
						static_cast<int>(texCoords.size()),
						static_cast<int>(normals.size()),
						key
					))
				{
					outMesh.Clear();
					outHeader.m_errorMessage = "Malformed OBJ face vertex";
					return false;
				}

				if (key.m_normalIndex < 0)
				{
					allFaceVerticesHaveNormals = false;
				}

				faceIndices.push_back(
					GetOrCreateVertex(key, positions, texCoords, normals, options, outMesh.m_vertices, vertexLookup)
				);
			}

			for (int faceIndex = 1; faceIndex < faceVertexCount - 1; ++faceIndex)
			{
				unsigned int i0 = faceIndices[0];
				unsigned int i1 = faceIndices[faceIndex];
				unsigned int i2 = faceIndices[faceIndex + 1];

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

			++outHeader.m_faceCount;
		}
	}

	outHeader.m_positionCount = static_cast<int>(positions.size());
	outHeader.m_texCoordCount = static_cast<int>(texCoords.size());
	outHeader.m_normalCount   = static_cast<int>(normals.size());
	outHeader.m_hasTexCoords  = !texCoords.empty();
	outHeader.m_hasNormals    = !normals.empty();

	if (outHeader.m_positionCount <= 0)
	{
		outMesh.Clear();
		outHeader.m_errorMessage = "OBJ file has no vertex positions";
		return false;
	}

	if (outHeader.m_faceCount <= 0 || outMesh.m_indices.empty())
	{
		outMesh.Clear();
		outHeader.m_errorMessage = "OBJ file has no faces";
		return false;
	}

	if (options.m_generateNormalsIfNone && !allFaceVerticesHaveNormals)
	{
		GenerateNormalsIfNeeded(outMesh);
	}

	if (options.m_uniformScale != 1.f)
	{
		for (Vertex& vertex : outMesh.m_vertices)
		{
			vertex.m_position *= options.m_uniformScale;
		}
	}

	TryLoadOBJTextures(filePath, materialLibraryPath, materialName, outMesh.m_texture);
	outHeader.m_isValid = true;
	return true;
}

bool OBJLoader::LoadFromFile(char const* filePath, MeshData& outMesh, OBJLoadOptions const& options)
{
	OBJHeader header;
	return LoadFromFile(filePath, outMesh, header, options);
}

void OBJLoader::GenerateNormalsIfNeeded(MeshData& mesh)
{
	for (Vertex& vertex : mesh.m_vertices)
	{
		vertex.m_normal = Vec3::Zero;
	}

	for (size_t index = 0; index + 2 < mesh.m_indices.size(); index += 3)
	{
		unsigned int const i0 = mesh.m_indices[index];
		unsigned int const i1 = mesh.m_indices[index + 1];
		unsigned int const i2 = mesh.m_indices[index + 2];

		if (i0 >= mesh.m_vertices.size() || i1 >= mesh.m_vertices.size() || i2 >= mesh.m_vertices.size())
		{
			continue;
		}

		Vec3 const edge01 = mesh.m_vertices[i1].m_position - mesh.m_vertices[i0].m_position;
		Vec3 const edge02 = mesh.m_vertices[i2].m_position - mesh.m_vertices[i0].m_position;
		Vec3       normal = CrossProduct3D(edge01, edge02);
		if (normal.GetLengthSquared() <= 0.f)
		{
			continue;
		}

		normal.Normalize();
		mesh.m_vertices[i0].m_normal += normal;
		mesh.m_vertices[i1].m_normal += normal;
		mesh.m_vertices[i2].m_normal += normal;
	}

	for (Vertex& vertex : mesh.m_vertices)
	{
		if (vertex.m_normal.GetLengthSquared() > 0.f)
		{
			vertex.m_normal.Normalize();
		}
	}
}
