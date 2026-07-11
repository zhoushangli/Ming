#pragma once

// Single public entry point for engine type registration.
// Calls RegisterCoreTypes, RegisterEngineTypes, and RegisterSceneTypes in order.
void RegisterAllTypes();

// Internal helpers — called by RegisterAllTypes only.
void RegisterCoreTypes();
void RegisterEngineTypes();
void RegisterSceneTypes();
