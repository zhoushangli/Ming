#pragma once

#include "MingEngine/Core/Math/FloatRange.hpp"
#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Math/IntVec3.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include "ThirdParty/tinyxml2/tinyxml2.h"

#include <string>

using XmlDocument  = tinyxml2::XMLDocument;
using XmlError     = tinyxml2::XMLError;
using XmlElement   = tinyxml2::XMLElement;
using XmlAttribute = tinyxml2::XMLAttribute;

int         ParseXmlAttribute(XmlElement const& element, char const* attributeName, int defaultValue);
char        ParseXmlAttribute(XmlElement const& element, char const* attributeName, char defaultValue);
bool        ParseXmlAttribute(XmlElement const& element, char const* attributeName, bool defaultValue);
float       ParseXmlAttribute(XmlElement const& element, char const* attributeName, float defaultValue);
Color       ParseXmlAttribute(XmlElement const& element, char const* attributeName, Color const& defaultValue);
Vector2     ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vector2 const& defaultValue);
Vector3     ParseXmlAttribute(XmlElement const& element, char const* attributeName, Vector3 const& defaultValue);
IntVec2     ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec2 const& defaultValue);
IntVec3     ParseXmlAttribute(XmlElement const& element, char const* attributeName, IntVec3 const& defaultValue);
std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, std::string const& defaultValue);
Strings     ParseXmlAttribute(
	XmlElement const& element, char const* attributeName, Strings const& defaultValues, char delimiter = ',');
std::string ParseXmlAttribute(XmlElement const& element, char const* attributeName, char const* defaultValue);
FloatRange  ParseXmlAttribute(XmlElement const& element, char const* attributeName, FloatRange const& defaultValue);
