#include "MingEngine/Core/XmlUtils.hpp"

#include <string>

int ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	return atoi(text);
}

char ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr || text[0] == '\0')
	{
		return defaultValue;
	}

	return text[0];
}

bool ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	std::string value(text);

	if (value == "true" || value == "1")
	{
		return true;
	}
	if (value == "false" || value == "0")
	{
		return false;
	}

	return defaultValue;
}

float ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	return (float)atof(text);
}

Color ParseXmlAttribute(XmlElement const& element, char const* attributeName, Color const& defaultValue)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	Color color;
	color.SetFromText(text);

	return color;
}

Vec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec2 const& defaultValue)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	Vec2 value;
	value.SetFromText(text);

	return value;
}

Vec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vec3 const& defaultValue)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	Strings parts = SplitStringOnDelimiter(std::string(text), ',');
	if (parts.size() < 3)
	{
		return defaultValue;
	}

	return Vec3(
		static_cast<float>(atof(parts[0].c_str())),
		static_cast<float>(atof(parts[1].c_str())),
		static_cast<float>(atof(parts[2].c_str())));
}

IntVec2 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	IntVec2 value;
	value.SetFromText(text);

	return value;
}

IntVec3 ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec3 const& defaultValue)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	IntVec3 value;
	value.SetFromText(text);

	return value;
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValue;
	}

	return std::string(text);
}

Strings ParseXmlAttribute(
	XmlElement const& element, char const* attributeName, Strings const& defaultValues, char delimiter /*= ','*/)
{
	char const* text = element.Attribute(attributeName);
	if (text == nullptr)
	{
		return defaultValues;
	}

	return SplitStringOnDelimiter(text, delimiter);
}

std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue)
{
	return ParseXmlAttribute(element, attributeName, std::string(defaultValue));
}

FloatRange ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange const& defaultValue)
{
	std::string value = ParseXmlAttribute(element, attributeName, "");
	if (value.empty())
	{
		return defaultValue;
	}

	Strings values = SplitStringOnDelimiter(value, '~');
	return FloatRange(static_cast<float>(atof(values[0].c_str())), static_cast<float>(atof(values[1].c_str())));
}
