#include "Engine/Core/XmlUtils.hpp"

#include <string>

int ParseXmlAttribute(XMLElement const& element, char const* attributeName, int defaultValue)
{
    char const* text = element.Attribute(attributeName);
    if (text == nullptr)
    {
        return defaultValue;
    }

    return atoi(text);
}

char ParseXmlAttribute(XMLElement const& element, char const* attributeName, char defaultValue)
{
    char const* text = element.Attribute(attributeName);
    if (text == nullptr || text[0] == '\0')
    {
        return defaultValue;
    }

    return text[0];
}

bool ParseXmlAttribute(XMLElement const& element, char const* attributeName, bool defaultValue)
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

float ParseXmlAttribute(XMLElement const& element, char const* attributeName, float defaultValue)
{
    char const* text = element.Attribute(attributeName);
    if (text == nullptr)
    {
        return defaultValue;
    }

    return (float)atof(text);
}

Rgba8 ParseXmlAttribute(XMLElement const& element, char const* attributeName, Rgba8 const& defaultValue)
{
    char const* text = element.Attribute(attributeName);
    if (text == nullptr)
    {
        return defaultValue;
    }

    Rgba8 color;
    color.SetFromText(text);

    return color;
}

Vec2 ParseXmlAttribute(XMLElement const& element, char const* attributeName, Vec2 const& defaultValue)
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

IntVec2 ParseXmlAttribute(XMLElement const& element, char const* attributeName, IntVec2 const& defaultValue)
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

std::string ParseXmlAttribute(XMLElement const& element, char const* attributeName, std::string const& defaultValue)
{
    char const* text = element.Attribute(attributeName);
    if (text == nullptr)
    {
        return defaultValue;
    }

    return std::string(text);
}

Strings ParseXmlAttribute(XMLElement const& element, char const* attributeName, Strings const& defaultValues, char delimiter /*= ','*/)
{
    char const* text = element.Attribute(attributeName);
    if (text == nullptr)
    {
        return defaultValues;
    }

    return SplitStringOnDelimiter(text, delimiter);
}

std::string ParseXmlAttribute(XMLElement const& element, char const* attributeName, char const* defaultValue)
{
    return ParseXmlAttribute(element, attributeName, std::string(defaultValue));
}
