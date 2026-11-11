#include "MingEngine/Core/Object/Callable.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/Object.hpp"

Callable::Callable(Object const* target, String const& methodName) : m_methodName(methodName)
{
	static_assert(std::is_standard_layout_v<Callable>);
	static_assert(sizeof(Callable) == 16 && alignof(Callable) == 8);
	static_assert(offsetof(Callable, m_targetID) == 0 && offsetof(Callable, m_methodName) == 8);

	if (target != nullptr)
	{
		m_targetID = target->GetObjectID();
	}
}

Object* Callable::GetTarget() const
{
	return ObjectDatabase::GetInstance(m_targetID);
}

bool Callable::IsValid() const
{
	Object* target = GetTarget();
	if (target == nullptr || target->IsPreDeleting())
	{
		return false;
	}

	return target->HasMethod(m_methodName.ToUtf8());
}

bool Callable::operator==(Callable const& other) const
{
	return m_targetID == other.m_targetID && m_methodName == other.m_methodName;
}

bool Callable::operator!=(Callable const& other) const
{
	return !(*this == other);
}

Variant Callable::Call(std::vector<Variant> const& arguments) const
{
	CallError error;
	Variant   result = Call(arguments, error);
	if (error.m_error != CallError::Error::Ok)
	{
		ERR_PRINT("Callable: failed to invoke '" + m_methodName.ToUtf8() + "'. Use Call(arguments, error) for details.");
	}

	return result;
}

Variant Callable::Call(std::vector<Variant> const& arguments, CallError& error) const
{
	error = {};

	// 1) Resolve the target each time without retaining the object
	Object* target = ObjectDatabase::GetInstance(m_targetID);
	if (target == nullptr)
	{
		error.m_error = CallError::Error::InvalidTarget;
		return {};
	}

	return target->Call(m_methodName.ToUtf8(), arguments, error);
}
