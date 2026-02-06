#pragma once

#include <vector>
#include <string>

int FileReadToBuffer(std::vector<uint8_t>& outBuffer, const std::string& filename);
int FileReadToString(std::string& outString, const std::string& filename);


	