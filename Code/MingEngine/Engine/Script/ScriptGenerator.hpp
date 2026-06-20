#pragma once

class asIScriptEngine;

constexpr char const* kScriptLibDirectory = "Data/ScriptLib";
constexpr char const* kGeneratedScriptFilename = "MingEngine.generated.as";
constexpr char const* kPredefinedScriptFilename = "as.predefined";
constexpr char const* kGeneratedScriptResourcePath = "res://ScriptLib/MingEngine.generated.as";

void GenerateBuiltinScript(asIScriptEngine* engine);