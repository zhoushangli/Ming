#include "MingEngine/Engine/Script/RegisterDynamicCast.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

#include "ThirdParty/angelscript/include/angelscript.h"

namespace
{
Node*   AsNativeNode(Object* object) { return dynamic_cast<Node*>(object); }
Node3D* AsNativeNode3D(Object* object) { return dynamic_cast<Node3D*>(object); }
} // namespace

void RegisterDynamicCast(asIScriptEngine* engine)
{
	int result = 0;

	result = engine->RegisterGlobalFunction(
		"NativeNode@ __AsNativeNode(NativeObject@)",
		asFUNCTION(AsNativeNode),
		asCALL_CDECL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register global function for dynamic cast: NativeObject to NativeNode");

	result = engine->RegisterGlobalFunction(
		"NativeNode3D@ __AsNativeNode3D(NativeObject@)",
		asFUNCTION(AsNativeNode3D),
		asCALL_CDECL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register global function for dynamic cast: NativeObject to NativeNode3D");
}