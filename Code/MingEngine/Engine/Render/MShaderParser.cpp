#include "MingEngine/Engine/Render/MShaderParser.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <unordered_set>

namespace
{
struct Token
{
	std::string m_text;
	size_t m_begin = 0;
	size_t m_end = 0;
	unsigned int m_line = 1;
	unsigned int m_column = 1;
};

bool IsIdentifier(std::string const& text)
{
	return !text.empty() && (std::isalpha(static_cast<unsigned char>(text[0])) || text[0] == '_');
}

std::vector<Token> Tokenize(std::string const& source)
{
	std::vector<Token> tokens;
	size_t position = source.compare(0, 3, "\xEF\xBB\xBF") == 0 ? 3 : 0;
	unsigned int line = 1;
	unsigned int column = 1;
	auto advance = [&]()
	{
		if (source[position++] == '\n')
		{
			++line;
			column = 1;
		}
		else
		{
			++column;
		}
	};
	while (position < source.size())
	{
		char character = source[position];
		if (std::isspace(static_cast<unsigned char>(character)))
		{
			advance();
			continue;
		}
		Token token { {}, position, position, line, column };
		if (source.compare(position, 2, "//") == 0 || character == '#')
		{
			bool continued = false;
			do
			{
				bool newline = source[position] == '\n';
				if (!newline)
				{
					continued = source[position] == '\\' || (continued && source[position] == '\r');
				}
				advance();
				if (newline && !continued)
				{
					break;
				}
			} while (position < source.size());
			continue;
		}
		if (source.compare(position, 2, "/*") == 0)
		{
			advance();
			advance();
			while (position < source.size() && source.compare(position, 2, "*/") != 0)
			{
				advance();
			}
			if (position == source.size())
			{
				throw std::runtime_error(std::to_string(token.m_line) + ":" + std::to_string(token.m_column) + ": unterminated comment");
			}
			advance();
			advance();
			continue;
		}
		if (character == '"' || character == '\'')
		{
			advance();
			bool closed = false;
			while (position < source.size())
			{
				char next = source[position];
				advance();
				if (next == '\\' && position < source.size())
				{
					advance();
				}
				else if (next == character)
				{
					closed = true;
					break;
				}
			}
			if (!closed)
			{
				throw std::runtime_error(std::to_string(token.m_line) + ":" + std::to_string(token.m_column) + ": unterminated string");
			}
		}
		else if (std::isalpha(static_cast<unsigned char>(character)) || character == '_')
		{
			do
			{
				advance();
			} while (position < source.size() && (std::isalnum(static_cast<unsigned char>(source[position])) || source[position] == '_'));
		}
		else if (std::isdigit(static_cast<unsigned char>(character)) || character == '.')
		{
			do
			{
				char previous = source[position];
				advance();
				if ((previous == 'e' || previous == 'E') && position < source.size() && (source[position] == '+' || source[position] == '-'))
				{
					advance();
				}
			} while (position < source.size() && (std::isalnum(static_cast<unsigned char>(source[position])) || source[position] == '.'));
		}
		else
		{
			advance();
		}
		token.m_end = position;
		token.m_text = source.substr(token.m_begin, position - token.m_begin);
		tokens.push_back(std::move(token));
	}
	tokens.push_back({ "", source.size(), source.size(), line, column });
	return tokens;
}
}

bool MShaderParser::Parse(std::string const& source, std::string const& path, MShaderParseResult& result, std::string& error)
{
	result = {};
	error.clear();
	try
	{
		auto tokens = Tokenize(source);
		size_t index = 0;
		auto fail = [&](std::string const& message)
		{
			Token const& token = tokens[index];
			throw std::runtime_error(std::to_string(token.m_line) + ":" + std::to_string(token.m_column) + ": " + message);
		};
		auto expect = [&](char const* text)
		{
			if (tokens[index].m_text != text)
			{
				fail(std::string("expected '") + text + "'");
			}
			++index;
		};
		auto erase = [&](size_t begin, size_t end)
		{
			for (size_t i = begin; i < end; ++i)
			{
				if (result.m_source[i] != '\n' && result.m_source[i] != '\r')
				{
					result.m_source[i] = ' ';
				}
			}
		};
		result.m_source = source;
		if (source.compare(0, 3, "\xEF\xBB\xBF") == 0)
		{
			erase(0, 3);
		}
		size_t begin = tokens[index].m_begin;
		expect("shader_type");
		expect("spatial");
		expect(";");
		erase(begin, tokens[index - 1].m_end);
		bool declarationsEnded = false;
		std::unordered_set<std::string> names;
		while (!tokens[index].m_text.empty())
		{
			begin = tokens[index].m_begin;
			if (tokens[index].m_text == "shader_type")
			{
				fail("shader_type must appear once at the start");
			}
			if (tokens[index].m_text == "uniform")
			{
				if (declarationsEnded)
				{
					fail("uniform declarations must precede HLSL definitions");
				}
				++index;
				std::string type = tokens[index].m_text;
				unsigned int components = type == "float" ? 1 : type == "float2" ? 2 : type == "float3" ? 3 : type == "float4" ? 4 : 0;
				if (components == 0)
				{
					fail("uniform type must be float, float2, float3 or float4");
				}
				++index;
				std::string name = tokens[index].m_text;
				if (!IsIdentifier(name) || name == "vertex" || name == "fragment" || !names.insert(name).second)
				{
					fail("invalid or duplicate uniform name");
				}
				++index;
				float values[4] = {};
				if (tokens[index].m_text == "=")
				{
					++index;
					if (components > 1)
					{
						expect(type.c_str());
						expect("(");
					}
					for (unsigned int component = 0; component < components; ++component)
					{
						float sign = 1.f;
						if (tokens[index].m_text == "-" || tokens[index].m_text == "+")
						{
							sign = tokens[index++].m_text == "-" ? -1.f : 1.f;
						}
						std::string const& number = tokens[index].m_text;
						char* end = nullptr;
						float value = std::strtof(number.c_str(), &end);
						if (end == number.c_str() || !std::isfinite(value) || (*end != '\0' && !((*end == 'f' || *end == 'F') && end[1] == '\0')))
						{
							fail("expected finite numeric literal");
						}
						values[component] = sign * value;
						++index;
						if (component + 1 < components)
						{
							expect(",");
						}
					}
					if (components > 1)
					{
						expect(")");
					}
				}
				expect(";");
				if (result.m_uniforms.size() >= 4096)
				{
					fail("material constants exceed 64 KiB");
				}
				Variant value = components == 1 ? Variant(values[0]) : components == 2 ? Variant(Vector2(values[0], values[1])) : components == 3 ? Variant(Vector3(values[0], values[1], values[2])) : Variant(Vector4(values[0], values[1], values[2], values[3]));
				unsigned int slot = static_cast<unsigned int>(result.m_uniforms.size());
				result.m_uniforms.push_back({ name, value, slot * 16, components });
				result.m_uniformSource += "    " + type + " " + name + " : packoffset(c" + std::to_string(slot) + ");\n";
				erase(begin, tokens[index - 1].m_end);
				continue;
			}
			declarationsEnded = true;
			size_t declaration = index;
			std::vector<std::string> delimiters;
			bool body = false;
			while (true)
			{
				std::string const& text = tokens[index].m_text;
				if (text.empty())
				{
					fail("unterminated top-level definition");
				}
				if (delimiters.empty() && (text == "{" || text == ";"))
				{
					for (size_t i = declaration; i < index; ++i)
					{
						if ((tokens[i].m_text == "vertex" || tokens[i].m_text == "fragment") && tokens[i + 1].m_text == "(")
						{
							bool vertex = tokens[i].m_text == "vertex";
							bool& present = vertex ? result.m_hasVertex : result.m_hasFragment;
							if (i != declaration + 1 || tokens[declaration].m_text != "void" || index != declaration + 7 || tokens[i + 2].m_text != "inout" || tokens[i + 3].m_text != (vertex ? "VertexContext" : "FragmentContext") || !IsIdentifier(tokens[i + 4].m_text) || tokens[i + 5].m_text != ")" || text != "{" || present)
							{
								fail("invalid or duplicate vertex/fragment callback; expected void callback(inout Context name) body");
							}
							present = true;
						}
					}
					if (text == ";")
					{
						++index;
						break;
					}
					body = true;
				}
				if (text == "(" || text == "[" || text == "{")
				{
					delimiters.push_back(text == "(" ? ")" : text == "[" ? "]" : "}");
				}
				else if (text == ")" || text == "]" || text == "}")
				{
					if (delimiters.empty() || delimiters.back() != text)
					{
						fail("mismatched delimiter");
					}
					delimiters.pop_back();
					if (body && delimiters.empty())
					{
						++index;
						if (tokens[index].m_text == ";")
						{
							++index;
						}
						break;
					}
				}
				++index;
			}
		}
		if (!result.m_uniforms.empty())
		{
			result.m_uniformSource = "cbuffer MShaderMaterialConstants : register(b6)\n{\n" + result.m_uniformSource + "};\n";
		}
		return true;
	}
	catch (std::exception const& exception)
	{
		error = path + ":" + exception.what();
		result = {};
		return false;
	}
}
