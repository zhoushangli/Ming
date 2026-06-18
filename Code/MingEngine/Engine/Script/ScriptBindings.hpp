#pragma once

class asIScriptEngine;

void RegisterVec3(asIScriptEngine* engine);
void RegisterEulerAngles(asIScriptEngine* engine);
void RegisterMatrix4x4(asIScriptEngine* engine);

void RegisterBridgeFunctions(asIScriptEngine* engine);
void GenerateBuiltinScript();