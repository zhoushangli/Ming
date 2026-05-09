#include "Engine/Core/StringUtils.hpp"

#include <stdarg.h>


constexpr int kStringfStackLocalTempLength = 2048;


const std::string Stringf(char const* format, ...)
{
    char textLiteral[kStringfStackLocalTempLength];
    va_list variableArgumentList;
    va_start(variableArgumentList, format);
    vsnprintf_s(textLiteral, kStringfStackLocalTempLength, _TRUNCATE, format, variableArgumentList);
    va_end(variableArgumentList);
    textLiteral[kStringfStackLocalTempLength - 1] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

    return std::string(textLiteral);
}


const std::string Stringf(int maxLength, char const* format, ...)
{
    char textLiteralSmall[kStringfStackLocalTempLength];
    char* textLiteral = textLiteralSmall;
    if (maxLength > kStringfStackLocalTempLength)
        textLiteral = new char[maxLength];

    va_list variableArgumentList;
    va_start(variableArgumentList, format);
    vsnprintf_s(textLiteral, maxLength, _TRUNCATE, format, variableArgumentList);
    va_end(variableArgumentList);
    textLiteral[maxLength - 1] = '\0'; // In case vsnprintf overran (doesn't auto-terminate)

    std::string returnValue(textLiteral);
    if (maxLength > kStringfStackLocalTempLength)
        delete[] textLiteral;

    return returnValue;
}

Strings SplitStringOnDelimiter(std::string const& originalString, char delimiterToSplitOn)
{
    Strings result;
    std::string currentPart;
    for (char ch : originalString)
    {
        if (ch == delimiterToSplitOn)
        {
            result.push_back(currentPart);
            currentPart.clear();
        }
        else
        {
            currentPart += ch;
        }
    }
    
    result.push_back(currentPart);
    return result;
}

