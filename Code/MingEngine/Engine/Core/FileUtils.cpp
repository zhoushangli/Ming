#include "MingEngine/Engine/Core/FileUtils.hpp"

#include <cstdio>   
#include <cerrno>   
#include <cstring>  
#include <cstdint>  
#include <vector>
#include <string>

int FileReadToBuffer(std::vector<uint8_t>& outBuffer, const std::string& filename)
{
    outBuffer.clear();

    FILE* file = nullptr;
    errno_t err = fopen_s(&file, filename.c_str(), "rb");
    if (err != 0 || file == nullptr)
    {
        return 0;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return 0;
    }

    long fileSize = ftell(file);
    if (fileSize < 0)
    {
        fclose(file);
        return 0;
    }

    if (fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return 0;
    }

    if (fileSize == 0)
    {
        fclose(file);
        return 0;
    }

    outBuffer.resize(static_cast<size_t>(fileSize));

    size_t bytesRead = fread(outBuffer.data(), 1, static_cast<size_t>(fileSize), file);
    fclose(file);

    outBuffer.resize(bytesRead);

    return static_cast<int>(bytesRead);
}

int FileReadToString(std::string& outString, const std::string& filename)
{
    outString.clear();

    std::vector<uint8_t> buffer;
    int bytesRead = FileReadToBuffer(buffer, filename);
    if (bytesRead <= 0)
    {
        return 0;
    }

    outString.resize(static_cast<size_t>(bytesRead));
    std::memcpy(outString.data(), buffer.data(), static_cast<size_t>(bytesRead));

    return bytesRead;
}
