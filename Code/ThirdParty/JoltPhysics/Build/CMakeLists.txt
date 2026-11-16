cmake_minimum_required(VERSION 3.20)
project(MingJolt LANGUAGES CXX)

set(JOLT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
file(GLOB_RECURSE JOLT_SOURCES CONFIGURE_DEPENDS "${JOLT_ROOT}/Jolt/*.cpp")
list(FILTER JOLT_SOURCES EXCLUDE REGEX "/Compute/(CPU|DX12|VK)/|/Shaders/.*Wrapper\\.cpp$")

add_library(Jolt STATIC ${JOLT_SOURCES})
target_include_directories(Jolt PUBLIC "${JOLT_ROOT}")
target_compile_features(Jolt PUBLIC cxx_std_17)
target_compile_definitions(Jolt PUBLIC
    JPH_FLOATING_POINT_EXCEPTIONS_ENABLED JPH_DEBUG_RENDERER JPH_PROFILE_ENABLED JPH_OBJECT_STREAM
    JPH_USE_AVX2 JPH_USE_AVX JPH_USE_SSE4_1 JPH_USE_SSE4_2 JPH_USE_LZCNT JPH_USE_TZCNT
    JPH_USE_F16C JPH_USE_FMADD)
target_compile_options(Jolt PRIVATE /arch:AVX2 /fp:precise /GR- /MP)
set_target_properties(Jolt PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    ARCHIVE_OUTPUT_DIRECTORY "${JOLT_ROOT}/Lib/x64/$<CONFIG>")
