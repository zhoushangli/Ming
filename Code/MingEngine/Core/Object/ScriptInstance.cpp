#include "MingEngine/Core/Object/ScriptInstance.hpp"

bool ScriptInstance::Call(std::string const& methodName, std::vector<Variant> const& args, Variant& ret)
{
	CallError error;
	ret = Call(methodName, args, error);
	return error.m_error == CallError::Error::Ok;
}
