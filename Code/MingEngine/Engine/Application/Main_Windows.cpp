#include "MingEngine/Engine/Application/Main_Windows.hpp"

#include "MingEngine/Engine/Application/App.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int WINAPI WinMain( [[maybe_unused]] HINSTANCE applicationInstanceHandle, HINSTANCE, [[maybe_unused]] LPSTR commandLineString, int)
{
	MingRunConfig config;
	return MingEngine::Run(config);
}
