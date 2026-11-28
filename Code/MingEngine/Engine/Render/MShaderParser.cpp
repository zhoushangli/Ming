#include "MingEngine/Engine/Render/MShaderParser.hpp"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cerrno>
#include <limits>
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

[[noreturn]] void Fail(std::vector<Token> const& tokens, size_t index, std::string const& message)
{
	Token const& token = tokens[index];
	throw std::runtime_error(std::to_string(token.m_line) + ":" + std::to_string(token.m_column) + ": " + message);
}

void Expect(std::vector<Token> const& tokens, size_t& index, char const* text)
{
	if (tokens[index].m_text != text)
	{
		Fail(tokens, index, std::string("expected '") + text + "'");
	}
	++index;
}

void EraseDeclaration(std::string& source, size_t begin, size_t end)
{
	for (size_t i = begin; i < end; ++i)
	{
		if (source[i] != '\n' && source[i] != '\r')
		{
			source[i] = ' ';
		}
	}
}

std::string ParseNumber(std::vector<Token> const& tokens, size_t& index, bool integer)
{
	std::string number;
	if (tokens[index].m_text == "-" || tokens[index].m_text == "+")
	{
		number = tokens[index++].m_text;
	}
	number += tokens[index].m_text;
	char* end = nullptr;
	errno = 0;
	if (integer)
	{
		long long value = std::strtoll(number.c_str(), &end, 10);
		if (end == number.c_str() || *end != '\0' || errno == ERANGE
			|| value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max())
		{
			Fail(tokens, index, "expected 32-bit integer literal");
		}
	}
	else
	{
		float value = std::strtof(number.c_str(), &end);
		if (end == number.c_str() || !std::isfinite(value)
			|| (*end != '\0' && !((*end == 'f' || *end == 'F') && end[1] == '\0')))
		{
			Fail(tokens, index, "expected finite numeric literal");
		}
	}
	if (!integer && !number.empty() && (number.back() == 'f' || number.back() == 'F'))
	{
		number.pop_back();
	}
	++index;
	return number;
}

Variant ParseUniformDefault(std::vector<Token> const& tokens, size_t& index, std::string const& type, unsigned int components)
{
	bool integer = type == "int";
	float values[4] = {};
	int integerValue = 0;
	if (tokens[index].m_text == "=")
	{
		++index;
		if (components > 1)
		{
			Expect(tokens, index, type.c_str());
			Expect(tokens, index, "(");
		}
		for (unsigned int component = 0; component < components; ++component)
		{
			std::string number = ParseNumber(tokens, index, integer);
			if (integer)
			{
				integerValue = static_cast<int>(std::strtoll(number.c_str(), nullptr, 10));
			}
			else
			{
				values[component] = std::strtof(number.c_str(), nullptr);
			}
			if (component + 1 < components)
			{
				Expect(tokens, index, ",");
			}
		}
		if (components > 1)
		{
			Expect(tokens, index, ")");
		}
	}
	if (integer)
	{
		return Variant(integerValue);
	}
	switch (components)
	{
	case 1: return Variant(values[0]);
	case 2: return Variant(Vector2(values[0], values[1]));
	case 3: return Variant(Vector3(values[0], values[1], values[2]));
	default: return Variant(Vector4(values[0], values[1], values[2], values[3]));
	}
}

void ParseUniformHint(std::vector<Token> const& tokens, size_t& index, MShaderUniform& uniform, std::string const& type)
{
	if (tokens[index].m_text != ":")
	{
		return;
	}
	++index;
	std::string hint = tokens[index++].m_text;
	if (hint == "hint_color" && type == "float4")
	{
		uniform.m_hint = PropertyInfo::Hint::Color;
	}
	else if (hint == "hint_range" && (type == "float" || type == "int"))
	{
		uniform.m_hint = PropertyInfo::Hint::Range;
		Expect(tokens, index, "(");
		std::string minimum = ParseNumber(tokens, index, type == "int");
		Expect(tokens, index, ",");
		std::string maximum = ParseNumber(tokens, index, type == "int");
		std::string step = type == "int" ? "1" : "0.001";
		if (tokens[index].m_text == ",")
		{
			++index;
			step = ParseNumber(tokens, index, type == "int");
		}
		if (std::strtod(minimum.c_str(), nullptr) > std::strtod(maximum.c_str(), nullptr)
			|| std::strtod(step.c_str(), nullptr) <= 0.0)
		{
			Fail(tokens, index, "hint_range requires min <= max and step > 0");
		}
		uniform.m_hintData = minimum + "," + maximum + "," + step;
		Expect(tokens, index, ")");
	}
	else if (hint == "hint_enum" && type == "int")
	{
		uniform.m_hint = PropertyInfo::Hint::Enum;
		Expect(tokens, index, "(");
		unsigned int option = 0;
		while (true)
		{
			std::string const& text = tokens[index].m_text;
			if (text.size() < 3 || text.front() != '"' || text.back() != '"'
				|| text.find_first_of(",:\\\r\n", 1) != std::string::npos)
			{
				Fail(tokens, index, "expected nonempty enum label without commas, colons or escapes");
			}
			if (!uniform.m_hintData.empty())
			{
				uniform.m_hintData += ",";
			}
			uniform.m_hintData += text.substr(1, text.size() - 2) + ":" + std::to_string(option++);
			++index;
			if (tokens[index].m_text != ",")
			{
				break;
			}
			++index;
		}
		Expect(tokens, index, ")");
	}
	else
	{
		Fail(tokens, index - 1, "unknown uniform hint or incompatible uniform type");
	}
}

void ParseUniformGroup(std::vector<Token> const& tokens, size_t& index, std::string& group)
{
	Expect(tokens, index, "group_uniforms");
	group.clear();
	if (tokens[index].m_text != ";")
	{
		if (!IsIdentifier(tokens[index].m_text))
		{
			Fail(tokens, index, "expected uniform group name");
		}
		group = tokens[index++].m_text;
	}
	Expect(tokens, index, ";");
}

void ParseUniform(std::vector<Token> const& tokens, size_t& index, MShaderParseResult& result,
	std::unordered_set<std::string>& names, std::string const& group)
{
	Expect(tokens, index, "uniform");
	std::string type = tokens[index].m_text;
	unsigned int components = type == "float" || type == "int" ? 1 : type == "float2" ? 2 : type == "float3" ? 3 : type == "float4" ? 4 : 0;
	if (components == 0)
	{
		Fail(tokens, index, "uniform type must be int, float, float2, float3 or float4");
	}
	++index;
	MShaderUniform uniform;
	uniform.m_name = tokens[index].m_text;
	if (!IsIdentifier(uniform.m_name) || uniform.m_name == "vertex" || uniform.m_name == "fragment" || !names.insert(uniform.m_name).second)
	{
		Fail(tokens, index, "invalid or duplicate uniform name");
	}
	++index;
	ParseUniformHint(tokens, index, uniform, type);
	uniform.m_defaultValue = ParseUniformDefault(tokens, index, type, components);
	if (uniform.m_hint == PropertyInfo::Hint::Color)
	{
		auto const& v = uniform.m_defaultValue.As<Vector4>();
		uniform.m_defaultValue = Color(v.x, v.y, v.z, v.w);
	}
	Expect(tokens, index, ";");
	if (result.m_uniforms.size() >= 4096)
	{
		Fail(tokens, index, "material constants exceed 64 KiB");
	}
	unsigned int slot = static_cast<unsigned int>(result.m_uniforms.size());
	uniform.m_offset = slot * 16;
	uniform.m_byteSize = type == "int" ? sizeof(int32_t) : components * sizeof(float);
	uniform.m_group = group;
	result.m_uniformSource += "    " + type + " " + uniform.m_name + " : packoffset(c" + std::to_string(slot) + ");\n";
	result.m_uniforms.push_back(std::move(uniform));
}

void SkipHlslDefinition(std::vector<Token> const& tokens, size_t& index, MShaderParseResult& result)
{
	size_t declaration = index;
	std::vector<std::string> delimiters;
	bool body = false;
	while (true)
	{
		std::string const& text = tokens[index].m_text;
		if (text.empty())
		{
			Fail(tokens, index, "unterminated top-level definition");
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
						Fail(tokens, index, "invalid or duplicate vertex/fragment callback; expected void callback(inout Context name) body");
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
				Fail(tokens, index, "mismatched delimiter");
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
}

bool MShaderParser::Parse(std::string const& source, std::string const& path, MShaderParseResult& result, std::string& error)
{
	result = {};
	error.clear();
	try
	{
		auto tokens = Tokenize(source);
		size_t index = 0;
		result.m_source = source;
		if (source.compare(0, 3, "\xEF\xBB\xBF") == 0)
		{
			EraseDeclaration(result.m_source, 0, 3);
		}
		size_t begin = tokens[index].m_begin;
		Expect(tokens, index, "shader_type");
		Expect(tokens, index, "spatial");
		Expect(tokens, index, ";");
		EraseDeclaration(result.m_source, begin, tokens[index - 1].m_end);

		bool declarationsEnded = false;
		std::unordered_set<std::string> names;
		std::string group;
		while (!tokens[index].m_text.empty())
		{
			begin = tokens[index].m_begin;
			std::string const& text = tokens[index].m_text;
			if (text == "shader_type")
			{
				Fail(tokens, index, "shader_type must appear once at the start");
			}
			if (text == "uniform" || text == "group_uniforms")
			{
				if (declarationsEnded)
				{
					Fail(tokens, index, "uniform declarations and groups must precede HLSL definitions");
				}
				if (text == "uniform")
				{
					ParseUniform(tokens, index, result, names, group);
				}
				else
				{
					ParseUniformGroup(tokens, index, group);
				}
				EraseDeclaration(result.m_source, begin, tokens[index - 1].m_end);
			}
			else
			{
				declarationsEnded = true;
				SkipHlslDefinition(tokens, index, result);
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
