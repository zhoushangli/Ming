#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Math/IntVec2.hpp"

#include "ThirdParty/tinyxml2/tinyxml2.h"

#include <string>

typedef tinyxml2::XMLDocument   XMLDocument;
typedef tinyxml2::XMLError      XMLError;
typedef tinyxml2::XMLElement    XMLElement;
typedef tinyxml2::XMLAttribute  XMLAttribute;
typedef tinyxml2::XMLError      XMLResult;

int         ParseXmlAttribute(XMLElement const& element, char const* attributeName, int defaultValue);
char        ParseXmlAttribute(XMLElement const& element, char const* attributeName, char defaultValue);
bool        ParseXmlAttribute(XMLElement const& element, char const* attributeName, bool defaultValue);
float       ParseXmlAttribute(XMLElement const& element, char const* attributeName, float defaultValue);
Rgba8       ParseXmlAttribute(XMLElement const& element, char const* attributeName, Rgba8 const& defaultValue);
Vec2        ParseXmlAttribute(XMLElement const& element, char const* attributeName, Vec2 const& defaultValue);
IntVec2     ParseXmlAttribute(XMLElement const& element, char const* attributeName, IntVec2 const& defaultValue);
std::string ParseXmlAttribute(XMLElement const& element, char const* attributeName, std::string const& defaultValue);
Strings     ParseXmlAttribute(XMLElement const& element, char const* attributeName, Strings const& defaultValues, char delimiter = ',');
std::string ParseXmlAttribute(XMLElement const& element, char const* attributeName, char const* defaultValue);
