#include "MingEngine/Engine/Core/NamedStrings.hpp"

#include "MingEngine/Engine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"

void NamedStrings::PopulateFromXmlElementAttributes(XmlElement const& element)
{
    XmlAttribute const* attribute = element.FirstAttribute();
    while (attribute)
    {
        DebuggerPrintf("[NamedStrings] Attribute: %-20s | Value: %s\n", attribute->Name(), attribute->Value());
        SetValue(attribute->Name(), attribute->Value());
        attribute = attribute->Next();
    }
}

void NamedStrings::SetValue(std::string const& keyName, std::string const& newValue)
{
    m_keyValuePairs[keyName] = newValue;
}

std::string NamedStrings::GetValue(std::string const& keyName, std::string const& defaultValue) const
{
    auto iter = m_keyValuePairs.find(keyName);
    if (iter != m_keyValuePairs.end())
    {
        return iter->second;
    }
    return defaultValue;
}

bool NamedStrings::GetValue(std::string const& keyName, bool defaultValue) const
{
    auto iter = m_keyValuePairs.find(keyName);
    if (iter != m_keyValuePairs.end())
    {
        std::string value = iter->second;
        if (value == "true" || value == "1")
        {
            return true;
        }
        if (value == "false" || value == "0")
        {
            return false;
        }
    }
    return defaultValue;
}

int NamedStrings::GetValue(std::string const& keyName, int defaultValue) const
{
    auto iter = m_keyValuePairs.find(keyName);
    if (iter != m_keyValuePairs.end())
    {
        return atoi(iter->second.c_str());
    }
    return defaultValue;
}

float NamedStrings::GetValue(std::string const& keyName, float defaultValue) const
{
    auto iter = m_keyValuePairs.find(keyName);
    if (iter != m_keyValuePairs.end())
    {
        return (float)atof(iter->second.c_str());
    }
    return defaultValue;
}

std::string NamedStrings::GetValue(std::string const& keyName, char const* defaultValue) const
{
    auto iter = m_keyValuePairs.find(keyName);
    if (iter != m_keyValuePairs.end())
    {
        return iter->second;
    }
    return std::string(defaultValue);
}

Rgba8 NamedStrings::GetValue(std::string const& keyName, Rgba8 const& defaultValue) const
{
    auto iter = m_keyValuePairs.find(keyName);
    if (iter != m_keyValuePairs.end())
    {
        Rgba8 result = defaultValue;
        result.SetFromText(iter->second.c_str());
        return result;
    }
    return defaultValue;
}

Vec2 NamedStrings::GetValue(std::string const& keyName, Vec2 const& defaultValue) const
{
    auto iter = m_keyValuePairs.find(keyName);
    if (iter != m_keyValuePairs.end())
    {
        Vec2 result = defaultValue;
        result.SetFromText(iter->second.c_str());
        return result;
    }
    return defaultValue;
}

IntVec2 NamedStrings::GetValue(std::string const& keyName, IntVec2 const& defaultValue) const
{
    auto iter = m_keyValuePairs.find(keyName);
    if (iter != m_keyValuePairs.end())
    {
        IntVec2 result = defaultValue;
        result.SetFromText(iter->second.c_str());
        return result;
    }
    return defaultValue;
}

std::string NamedStrings::GetDebugInfo() const
{
    std::string debugInfo;
    for (auto const& pair : m_keyValuePairs)
    {
        debugInfo += Stringf("Key: %-10s | Value: %s", pair.first.c_str(), pair.second.c_str());
    }
    return debugInfo;
}

void NamedStrings::DebugPrintContents()
{
    DebuggerPrintf("NamedStrings Contents:\n");
    for (auto const& pair : m_keyValuePairs)
    {
        DebuggerPrintf("  Key: %-20s | Value: %s\n", pair.first.c_str(), pair.second.c_str());
    }
}
