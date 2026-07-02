#include "MingEngine/Scene/Import/OBJImporter.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"

#include <cstdlib>

namespace
{
struct FaceData
{
	int m_positionIndex = -1;
	int m_texCoordIndex = -1;
	int m_normalIndex   = -1;
};

struct OBJData
{
	// Source Data
	std::string       m_name;
	std::string       m_materialLibraryPath;
	std::vector<Vec3> m_positions;
	std::vector<Vec3> m_normals;
	std::vector<Vec2> m_texCoords;

	std::vector<FaceData> m_faces;

	// Engine Data
	std::vector<Vertex>       m_vertices;
	std::vector<unsigned int> m_indices;
};

bool ParseOBJFile(std::string const& sourceVirtualPath, OBJData& outData)
{
	auto clearOutput = [&outData]()
	{
		outData.m_name.clear();
		outData.m_materialLibraryPath.clear();
		outData.m_positions.clear();
		outData.m_normals.clear();
		outData.m_texCoords.clear();
		outData.m_faces.clear();
		outData.m_vertices.clear();
		outData.m_indices.clear();
	};

	auto trimWhitespace = [](std::string const& text) -> std::string
	{
		size_t const start = text.find_first_not_of(" \t\r\n");
		if (start == std::string::npos)
		{
			return "";
		}

		size_t const end = text.find_last_not_of(" \t\r\n");
		return text.substr(start, end - start + 1);
	};

	auto beginsWith = [](std::string const& text, char const* prefix) -> bool
	{
		size_t prefixLength = 0;
		while (prefix[prefixLength] != '\0')
		{
			++prefixLength;
		}

		return text.size() >= prefixLength && text.compare(0, prefixLength, prefix) == 0;
	};

	auto tokenize = [](std::string const& line) -> std::vector<std::string>
	{
		std::vector<std::string> tokens;
		size_t                   cursor = 0;
		while (cursor < line.size())
		{
			while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t'))
			{
				++cursor;
			}

			size_t const tokenStart = cursor;
			while (cursor < line.size() && line[cursor] != ' ' && line[cursor] != '\t')
			{
				++cursor;
			}

			if (cursor > tokenStart)
			{
				tokens.push_back(line.substr(tokenStart, cursor - tokenStart));
			}
		}

		return tokens;
	};

	auto resolveOBJIndex = [](int rawIndex, int count) -> int
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
	};

	auto splitFaceVertex = [](std::string const& token) -> std::vector<std::string>
	{
		std::vector<std::string> parts;
		size_t                   partStart = 0;
		while (partStart <= token.size())
		{
			size_t const slash = token.find('/', partStart);
			if (slash == std::string::npos)
			{
				parts.push_back(token.substr(partStart));
				break;
			}

			parts.push_back(token.substr(partStart, slash - partStart));
			partStart = slash + 1;
		}

		return parts;
	};

	auto parseFaceVertex = [&resolveOBJIndex, &splitFaceVertex](
							   std::string const& token,
							   int                positionCount,
							   int                texCoordCount,
							   int                normalCount,
							   FaceData&          outFaceVertex) -> bool
	{
		outFaceVertex = FaceData();

		std::vector<std::string> const parts = splitFaceVertex(token);
		if (parts.empty() || parts[0].empty())
		{
			return false;
		}

		outFaceVertex.m_positionIndex = resolveOBJIndex(atoi(parts[0].c_str()), positionCount);
		if (outFaceVertex.m_positionIndex < 0 || outFaceVertex.m_positionIndex >= positionCount)
		{
			return false;
		}

		if (parts.size() >= 2 && !parts[1].empty())
		{
			outFaceVertex.m_texCoordIndex = resolveOBJIndex(atoi(parts[1].c_str()), texCoordCount);
			if (outFaceVertex.m_texCoordIndex < 0 || outFaceVertex.m_texCoordIndex >= texCoordCount)
			{
				return false;
			}
		}

		if (parts.size() >= 3 && !parts[2].empty())
		{
			outFaceVertex.m_normalIndex = resolveOBJIndex(atoi(parts[2].c_str()), normalCount);
			if (outFaceVertex.m_normalIndex < 0 || outFaceVertex.m_normalIndex >= normalCount)
			{
				return false;
			}
		}

		return true;
	};

	auto faceVerticesAreEqual = [](FaceData const& a, FaceData const& b) -> bool
	{
		return a.m_positionIndex == b.m_positionIndex && a.m_texCoordIndex == b.m_texCoordIndex
			   && a.m_normalIndex == b.m_normalIndex;
	};

	auto crossProduct = [](Vec3 const& a, Vec3 const& b) -> Vec3
	{ return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); };

	clearOutput();

	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr)
	{
		return false;
	}

	std::string text;
	if (!g_engine->m_fileSystem->ReadText(sourceVirtualPath, text) || text.empty())
	{
		return false;
	}

	if (text.size() >= 2)
	{
		unsigned int const firstBytes =
			static_cast<unsigned char>(text[0]) | (static_cast<unsigned char>(text[1]) << 8);
		if (firstBytes == 0x0 || firstBytes == 0x8664 || firstBytes == 0x1c0 || firstBytes == 0x14c
			|| firstBytes == 0x200)
		{
			clearOutput();
			return false;
		}
	}

	std::vector<Rgba8>    colors;
	std::vector<FaceData> uniqueFaceVertices;
	bool                  allFaceVerticesHaveNormals = true;

	size_t cursor = 0;
	while (cursor <= text.size())
	{
		size_t lineEnd = text.find('\n', cursor);
		if (lineEnd == std::string::npos)
		{
			lineEnd = text.size();
		}

		std::string line = trimWhitespace(text.substr(cursor, lineEnd - cursor));
		cursor           = lineEnd + 1;

		while (!line.empty() && line.back() == '\\' && cursor <= text.size())
		{
			line.pop_back();
			size_t continuationEnd = text.find('\n', cursor);
			if (continuationEnd == std::string::npos)
			{
				continuationEnd = text.size();
			}

			line += trimWhitespace(text.substr(cursor, continuationEnd - cursor));
			cursor = continuationEnd + 1;
		}

		if (line.empty() || line[0] == '#')
		{
			continue;
		}

		std::vector<std::string> const tokens = tokenize(line);
		if (tokens.empty())
		{
			continue;
		}

		if (tokens[0] == "v")
		{
			if (tokens.size() < 4)
			{
				clearOutput();
				return false;
			}

			outData.m_positions.emplace_back(
				static_cast<float>(atof(tokens[1].c_str())),
				static_cast<float>(atof(tokens[2].c_str())),
				static_cast<float>(atof(tokens[3].c_str())));

			if (tokens.size() >= 7)
			{
				while (colors.size() + 1 < outData.m_positions.size())
				{
					colors.push_back(Rgba8::White);
				}

				colors.emplace_back(
					DenormalizeByte(static_cast<float>(atof(tokens[4].c_str()))),
					DenormalizeByte(static_cast<float>(atof(tokens[5].c_str()))),
					DenormalizeByte(static_cast<float>(atof(tokens[6].c_str()))),
					static_cast<unsigned char>(255));
			}
			else if (!colors.empty())
			{
				colors.push_back(Rgba8::White);
			}

			continue;
		}

		if (tokens[0] == "vt")
		{
			if (tokens.size() < 3)
			{
				clearOutput();
				return false;
			}

			outData.m_texCoords.emplace_back(
				static_cast<float>(atof(tokens[1].c_str())),
				1.f - static_cast<float>(atof(tokens[2].c_str())));
			continue;
		}

		if (tokens[0] == "vn")
		{
			if (tokens.size() < 4)
			{
				clearOutput();
				return false;
			}

			outData.m_normals.emplace_back(
				static_cast<float>(atof(tokens[1].c_str())),
				static_cast<float>(atof(tokens[2].c_str())),
				static_cast<float>(atof(tokens[3].c_str())));
			continue;
		}

		if (tokens[0] == "f")
		{
			if (tokens.size() < 4)
			{
				clearOutput();
				return false;
			}

			size_t const          firstFacePartCount = splitFaceVertex(tokens[1]).size();
			std::vector<FaceData> faceVertices;
			faceVertices.reserve(tokens.size() - 1);

			for (size_t tokenIndex = 1; tokenIndex < tokens.size(); ++tokenIndex)
			{
				if (splitFaceVertex(tokens[tokenIndex]).size() != firstFacePartCount)
				{
					clearOutput();
					return false;
				}

				FaceData faceVertex;
				if (!parseFaceVertex(
						tokens[tokenIndex],
						static_cast<int>(outData.m_positions.size()),
						static_cast<int>(outData.m_texCoords.size()),
						static_cast<int>(outData.m_normals.size()),
						faceVertex))
				{
					clearOutput();
					return false;
				}

				if (faceVertex.m_normalIndex < 0)
				{
					allFaceVerticesHaveNormals = false;
				}

				faceVertices.push_back(faceVertex);
			}

			for (size_t faceIndex = 1; faceIndex + 1 < faceVertices.size(); ++faceIndex)
			{
				FaceData const triangle[3] = {
					faceVertices[faceIndex],
					faceVertices[0],
					faceVertices[faceIndex + 1],
				};

				for (FaceData const& faceVertex : triangle)
				{
					outData.m_faces.push_back(faceVertex);

					unsigned int vertexIndex = 0;
					bool         foundVertex = false;
					for (size_t existingIndex = 0; existingIndex < uniqueFaceVertices.size(); ++existingIndex)
					{
						if (faceVerticesAreEqual(uniqueFaceVertices[existingIndex], faceVertex))
						{
							vertexIndex = static_cast<unsigned int>(existingIndex);
							foundVertex = true;
							break;
						}
					}

					if (!foundVertex)
					{
						Vertex vertex(outData.m_positions[faceVertex.m_positionIndex], Rgba8::White);
						if (faceVertex.m_positionIndex < static_cast<int>(colors.size()))
						{
							vertex.m_color = colors[faceVertex.m_positionIndex];
						}

						if (faceVertex.m_texCoordIndex >= 0)
						{
							vertex.m_uvTexCoords = outData.m_texCoords[faceVertex.m_texCoordIndex];
						}

						if (faceVertex.m_normalIndex >= 0)
						{
							vertex.m_normal = outData.m_normals[faceVertex.m_normalIndex];
						}

						vertexIndex = static_cast<unsigned int>(outData.m_vertices.size());
						outData.m_vertices.push_back(vertex);
						uniqueFaceVertices.push_back(faceVertex);
					}

					outData.m_indices.push_back(vertexIndex);
				}
			}

			continue;
		}

		if (tokens[0] == "o")
		{
			outData.m_name = trimWhitespace(line.substr(1));
			continue;
		}

		if (tokens[0] == "mtllib")
		{
			outData.m_materialLibraryPath = trimWhitespace(line.substr(6));
			continue;
		}

		if (beginsWith(line, "s "))
		{
			continue;
		}
	}

	if (outData.m_positions.empty() || outData.m_vertices.empty() || outData.m_indices.empty())
	{
		clearOutput();
		return false;
	}

	if (outData.m_name.empty())
	{
		outData.m_name = "Mesh";
	}

	if (!allFaceVerticesHaveNormals)
	{
		for (Vertex& vertex : outData.m_vertices)
		{
			vertex.m_normal = Vec3::Zero;
		}

		for (size_t index = 0; index + 2 < outData.m_indices.size(); index += 3)
		{
			unsigned int const i0 = outData.m_indices[index];
			unsigned int const i1 = outData.m_indices[index + 1];
			unsigned int const i2 = outData.m_indices[index + 2];
			if (i0 >= outData.m_vertices.size() || i1 >= outData.m_vertices.size() || i2 >= outData.m_vertices.size())
			{
				continue;
			}

			Vec3 const edge01 = outData.m_vertices[i1].m_position - outData.m_vertices[i0].m_position;
			Vec3 const edge02 = outData.m_vertices[i2].m_position - outData.m_vertices[i0].m_position;
			Vec3       normal = crossProduct(edge01, edge02);
			if (normal.GetLengthSquared() <= 0.f)
			{
				continue;
			}

			normal.Normalize();
			outData.m_vertices[i0].m_normal += normal;
			outData.m_vertices[i1].m_normal += normal;
			outData.m_vertices[i2].m_normal += normal;
		}

		for (Vertex& vertex : outData.m_vertices)
		{
			if (vertex.m_normal.GetLengthSquared() > 0.f)
			{
				vertex.m_normal.Normalize();
			}
		}
	}

	return true;
}
} // namespace

std::vector<std::string> OBJImporter::GetSupportedExtensions() const { return { ".obj" }; }

bool OBJImporter::Import(std::string const& sourceVirtualPath, std::string const& importVirtualPath)
{
	ImportMeshData meshData;

	OBJData objData;
	if (!ParseOBJFile(sourceVirtualPath, objData))
	{
		return false;
	}

	// 1) Copy OBJData into ImportMeshData
	meshData.m_name         = objData.m_name;
	meshData.m_vertexFormat = "PCUTBN";
	meshData.m_vertexStride = sizeof(Vertex);
	meshData.m_vertexCount  = static_cast<uint32_t>(objData.m_vertices.size());
	meshData.m_vertices.resize(meshData.m_vertexCount * meshData.m_vertexStride);
	memcpy(meshData.m_vertices.data(), objData.m_vertices.data(), meshData.m_vertices.size());

	meshData.m_indexFormat = "uint32";
	meshData.m_indexStride = sizeof(uint32_t);
	meshData.m_indexCount  = static_cast<uint32_t>(objData.m_indices.size());
	meshData.m_indices.resize(meshData.m_indexCount * sizeof(uint32_t));
	memcpy(meshData.m_indices.data(), objData.m_indices.data(), meshData.m_indices.size());

	return false;
};
