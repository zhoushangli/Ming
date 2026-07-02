#pragma once

class asIScriptEngine;

constexpr char const* kScriptLibDirectory = "Data/.ming/ScriptLib";
constexpr char const* kGeneratedScriptFilename = "MingEngine.generated.as";
constexpr char const* kPredefinedScriptFilename = "as.predefined";
constexpr char const* kGeneratedScriptResourcePath = "res://.ming/ScriptLib/MingEngine.generated.as";

void GenerateBuiltinScript(asIScriptEngine* engine);
