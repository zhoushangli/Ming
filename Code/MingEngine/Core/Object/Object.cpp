#include "MingEngine/Core/Object/Object.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/Callable.hpp"
#include "MingEngine/Core/Object/Signal.hpp"
#include "MingEngine/Core/Object/NativeScript.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/ScriptInstance.hpp"
#include "MingEngine/Core/Object/Variant.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include <algorithm>

namespace
{
MethodInfo const* FindNativeMethod(std::string const& className, std::string const& methodName)
{
	ClassInfo const* classInfo = ClassDatabase::GetClassInfo(className);
	while (classInfo != nullptr)
	{
		for (MethodInfo const& method : classInfo->m_methods)
		{
			if (method.m_name == methodName && method.m_bind != nullptr)
			{
				return &method;
			}
		}

		classInfo = ClassDatabase::GetClassInfo(classInfo->m_parentClassName);
	}

	return nullptr;
}
} // namespace

struct Object::Connection
{
	Signal   m_signal;
	Callable m_callable;
};

std::vector<Object*> ObjectDatabase::m_objectSlots;
uint32_t             ObjectDatabase::m_nextObjectUID = 1u;

Object::Object()
{
	ObjectDatabase::AddInstance(this);
}

Object::~Object()
{
	m_isPreDeleting = true;
	ClearSignalConnections();
	ReleaseNativeBindingGCHandle();
	ObjectDatabase::RemoveInstance(this);
}

void PostInitializeHandler(Object* object)
{
	if (object == nullptr)
	{
		return;
	}

	object->PostInitialize();
}

bool PreDeleteHandler(Object* object)
{
	if (object == nullptr)
	{
		return true;
	}

	return object->PreDelete();
}

void Object::PostInitialize()
{
	Notification(Notification_PostInitialize, false);
}

bool Object::PreDelete()
{
	if (m_isPreDeleting)
	{
		return false;
	}

	m_isPreDeleting = true;

	Notification(Notification_PreDelete, true);
	Notification(Notification_PreDeleteCleanup, true);

	ReleaseNativeBindingGCHandle();
	SetScriptInstance(nullptr);

	return true;
}

Object::BindMethodsFunc Object::GetBindMethodsFunc()
{
	return &Object::BindMethods;
}

bool Object::HasMethod(std::string const& methodName) const
{
	if (m_scriptInstance != nullptr && m_scriptInstance->HasMethod(methodName))
	{
		return true;
	}

	return FindNativeMethod(GetClassName(), methodName) != nullptr;
}

Variant Object::Call(std::string const& methodName, std::vector<Variant> const& arguments)
{
	CallError error;
	Variant result = Call(methodName, arguments, error);
	if (error.m_error != CallError::Error::Ok)
	{
		ERR_PRINT("Object: failed to invoke '" + methodName + "'. Use Call(methodName, arguments, error) for details.");
	}

	return result;
}

Variant Object::Call(std::string const& methodName, std::vector<Variant> const& arguments, CallError& error)
{
	error = {};

	// 1) Only a missing script method permits native fallback
	if (m_scriptInstance != nullptr)
	{
		Variant result = m_scriptInstance->Call(methodName, arguments, error);
		if (error.m_error != CallError::Error::InvalidMethod)
		{
			return result;
		}
	}

	error = {};

	// 2) Find the native binding and its declared argument types
	MethodInfo const* method = FindNativeMethod(GetClassName(), methodName);
	if (method == nullptr || method->m_bind == nullptr)
	{
		error.m_error = CallError::Error::InvalidMethod;
		return {};
	}

	// 3) Validate argument counts and types before MethodBind reads their payloads
	if (arguments.size() != method->m_argumentInfos.size())
	{
		error.m_error = arguments.size() < method->m_argumentInfos.size() ? CallError::Error::TooFewArguments
																		  : CallError::Error::TooManyArguments;
		error.m_expectedArgumentCount = static_cast<int>(method->m_argumentInfos.size());
		return {};
	}

	for (size_t index = 0; index < arguments.size(); ++index)
	{
		ArgumentInfo const& expected = method->m_argumentInfos[index];
		Variant const&      argument = arguments[index];
		if (expected.m_type == Variant::Type::Any)
		{
			continue;
		}

		bool valid = argument.GetType() == expected.m_type;
		if (valid && expected.m_type == Variant::Type::ObjectPtr && !expected.m_objectClassName.empty())
		{
			Object* argumentObject = argument.As<Object*>();
			valid = argumentObject == nullptr
					|| ClassDatabase::IsSubclassOf(argumentObject->GetClassName(), expected.m_objectClassName);
		}

		if (!valid)
		{
			error.m_error        = CallError::Error::InvalidArgument;
			error.m_argument     = static_cast<int>(index);
			error.m_expectedType = expected.m_type;
			return {};
		}
	}

	// 4) Invoke with this call's arguments and return the method's result
	return method->m_bind->Invoke(this, arguments);
}

void Object::BindMethods()
{
	ClassDatabase::BindMethod("SetScript", &Object::SetScript, { "script" });
	ClassDatabase::BindMethod("GetScript", &Object::GetScript, {});
	ClassDatabase::BindMethod("Free", &Object::Free, {});

	ADD_PROPERTY(
		PropertyInfo(
			Variant::Type::ObjectPtr,
			"script",
			PropertyInfo::Hint::ResourceType,
			Script::GetStaticClassName(),
			PropertyInfo::UsageFlags::Default),
		"SetScript",
		"GetScript");
}

std::string Object::GetStaticClassName()
{
	return "Object";
}

std::string Object::GetClassName() const
{
	return "Object";
}

ObjectID Object::GetObjectID() const
{
	return m_id;
}

void Object::InitializeClass()
{
	static bool isClassInitialized = false;
	if (isClassInitialized)
	{
		return;
	}

	isClassInitialized = true;
	BindMethods();
}

void Object::Notification(int notification, bool reverse)
{
	if (reverse)
	{
		NotificationBackward(notification);
	}
	else
	{
		NotificationForward(notification);
	}
}

void Object::Free()
{
	ERR_FAIL_COND_MSG(dynamic_cast<RefCounted*>(this) != nullptr,
		"RefCounted objects must be released through their references.");

	MemDelete(this);
}

bool Object::Connect(std::string const& signalName, Callable const& callable)
{
	if (m_isPreDeleting || signalName.empty() || !callable.IsValid() || IsConnected(signalName, callable))
	{
		return false;
	}

	Connection connection{ Signal(this, signalName), callable };
	Object*    target = callable.GetTarget();
	m_signalMap[signalName].push_back(connection);
	if (target != nullptr)
	{
		target->m_signalSubscriptions.push_back(connection);
	}
	return true;
}

bool Object::Disconnect(std::string const& signalName, Callable const& callable)
{
	auto signal = m_signalMap.find(signalName);
	if (signal == m_signalMap.end())
	{
		return false;
	}

	auto& connections = signal->second;
	auto  found       = std::find_if(
		connections.begin(),
		connections.end(),
		[&callable](Connection const& connection)
		{
			return connection.m_callable == callable;
		});
	if (found == connections.end())
	{
		return false;
	}

	Connection connection = *found;
	Object*    target     = connection.m_callable.GetTarget();
	if (target != nullptr)
	{
		auto& subscriptions = target->m_signalSubscriptions;
		subscriptions.erase(
			std::remove_if(
				subscriptions.begin(),
				subscriptions.end(),
				[&connection](Connection const& subscription)
				{
					return subscription.m_signal == connection.m_signal
						   && subscription.m_callable == connection.m_callable;
				}),
			subscriptions.end());
	}

	connections.erase(found);
	if (connections.empty())
	{
		m_signalMap.erase(signal);
	}

	return true;
}

bool Object::IsConnected(std::string const& signalName, Callable const& callable) const
{
	auto signal = m_signalMap.find(signalName);
	if (signal == m_signalMap.end())
	{
		return false;
	}

	auto const& connections = signal->second;
	return std::any_of(
		connections.begin(),
		connections.end(),
		[&callable](Connection const& connection)
		{
			return connection.m_callable == callable;
		});
}

bool Object::HasSignalConnections(std::string const& signalName) const
{
	return m_signalMap.find(signalName) != m_signalMap.end();
}

bool Object::EmitSignal(std::string const& signalName)
{
	return EmitSignal(signalName, {});
}

bool Object::EmitSignal(std::string const& signalName, std::vector<Variant> const& arguments)
{
	if (m_isPreDeleting || signalName.empty())
	{
		return false;
	}

	auto signal = m_signalMap.find(signalName);
	if (signal == m_signalMap.end())
	{
		return true;
	}

	// 1) Copy connections and arguments before callbacks can change or destroy their owners
	std::vector<Connection> const connections   = signal->second;
	std::vector<Variant> const    callArguments = arguments;
	ObjectID const                sourceID      = m_id;
	bool                          success       = true;

	// 2) Invoke the snapshot without keeping references into either object's connection tables
	for (Connection const& connection : connections)
	{
		if (connection.m_callable.IsManaged() && !connection.m_callable.IsValid())
		{
			Disconnect(signalName, connection.m_callable);
			continue;
		}

		Object* target = connection.m_callable.GetTarget();
		if ((!connection.m_callable.IsManaged() && target == nullptr)
			|| (target != nullptr && target->IsPreDeleting()))
		{
			continue;
		}

		Callable::CallError error;
		connection.m_callable.Call(callArguments, error);
		if (error.m_error != Callable::CallError::Error::Ok)
		{
			ERR_PRINT(
				"Signal: failed to invoke '" + (connection.m_callable.IsManaged() ? std::string("Delegate::Invoke")
					: connection.m_callable.GetMethodName().ToUtf8()) + "' from '"
				+ connection.m_signal.GetName() + "'.");
			success = false;
		}

		Object* source = ObjectDatabase::GetInstance(sourceID);
		if (source == nullptr || source->IsPreDeleting())
		{
			break;
		}
	}

	return success;
}

void Object::ClearSignalConnections()
{
	// 1) Remove outgoing connections and their records on each receiver
	while (!m_signalMap.empty())
	{
		Connection connection = m_signalMap.begin()->second.back();
		Disconnect(connection.m_signal.GetName(), connection.m_callable);
	}

	// 2) Ask each emitter to remove the subscriptions that target this object
	while (!m_signalSubscriptions.empty())
	{
		Connection connection = m_signalSubscriptions.back();
		if (!connection.m_signal.Disconnect(connection.m_callable))
		{
			m_signalSubscriptions.pop_back();
		}
	}
}

void Object::NotificationForward(int notification)
{
	NotificationForwardV(notification);
	if (m_scriptInstance)
	{
		m_scriptInstance->Notification(notification, false);
	}
}

void Object::NotificationBackward(int notification)
{
	if (m_scriptInstance)
	{
		m_scriptInstance->Notification(notification, true);
	}

	NotificationBackwardV(notification);
}

ObjectID ObjectDatabase::AddInstance(Object* object)
{
	if (object == nullptr)
	{
		return ObjectID::Invalid;
	}

	if (object->m_id.IsValid() && GetInstance(object->m_id) == object)
	{
		return object->m_id;
	}

	size_t index = 0;
	for (; index < m_objectSlots.size(); ++index)
	{
		if (m_objectSlots[index] == nullptr)
		{
			break;
		}
	}

	if (index > ObjectID::kMaxIndex)
	{
		return ObjectID::Invalid;
	}

	if (index == m_objectSlots.size())
	{
		m_objectSlots.push_back(object);
	}
	else
	{
		m_objectSlots[index] = object;
	}

	uint32_t const uid = m_nextObjectUID;
	m_nextObjectUID    = uid == ObjectID::kMaxUID ? 1u : uid + 1u;

	object->m_id = ObjectID(uid, static_cast<uint32_t>(index));
	return object->m_id;
}

void ObjectDatabase::RemoveInstance(Object* object)
{
	if (object == nullptr || !object->m_id.IsValid())
	{
		return;
	}

	uint32_t const index = object->m_id.GetIndex();
	if (index < m_objectSlots.size() && m_objectSlots[index] == object)
	{
		m_objectSlots[index] = nullptr;
	}

	object->m_id = ObjectID::Invalid;
}

Object* ObjectDatabase::GetInstance(ObjectID handle)
{
	if (!handle.IsValid())
	{
		return nullptr;
	}

	uint32_t const index = handle.GetIndex();
	if (index >= m_objectSlots.size())
	{
		return nullptr;
	}

	Object* object = m_objectSlots[index];
	if (object == nullptr || object->m_id != handle)
	{
		return nullptr;
	}

	return object;
}

Variant Object::GetScript() const
{
	if (!m_scriptInstance)
	{
		return Variant(static_cast<Object*>(nullptr));
	}

	return Variant(this->m_scriptInstance->GetScript().Get());
}

void Object::SetScript(Variant const& script)
{
	ERR_FAIL_COND_MSG(
		g_engine->m_scriptSystem->IsScriptExecutionSuspended(),
		"Cannot replace a script while reload is incomplete.\n");

	Ref<Script> scriptRef = script;
	if (dynamic_cast<CSharpScript*>(scriptRef.Get()) != nullptr && !g_engine->IsEditorMode())
	{
		std::string const base = g_engine->m_scriptSystem->GetScriptNativeBase(scriptRef->GetVirtualPath());
		ERR_FAIL_COND_MSG(base.empty() || !ClassDatabase::IsSubclassOf(GetClassName(), base),
			"Script is missing a registered type or has an incompatible native base: " + scriptRef->GetVirtualPath().GetString());
	}

	if (!scriptRef.IsValid())
	{
		SetScriptInstance(nullptr);
		return;
	}

	ScriptInstance* previous = std::exchange(m_scriptInstance, nullptr);
	bool const created = g_engine->IsEditorMode() ? scriptRef->InstantiatePlaceHolder(this) : scriptRef->Instantiate(this);
	if (!created)
	{
		SetScriptInstance(nullptr);
		m_scriptInstance = previous;
		ERR_PRINT("Failed to instantiate script " + scriptRef->GetVirtualPath().GetString() + " for " + GetClassName());
	}
	else
	{
		MemDelete(previous);
	}

	if (dynamic_cast<CSharpInstance*>(m_scriptInstance) != nullptr)
	{
		g_engine->m_scriptSystem->RegisterScriptOwner(GetObjectID());
	}

}

void Object::SetScriptInstance(ScriptInstance* instance)
{
	if (m_scriptInstance == instance)
	{
		return;
	}

	GUARANTEE_OR_DIE(
		instance == nullptr || instance->GetOwner() == this,
		"Object::SetScriptInstance failed: script instance owner mismatch.");

	ScriptInstance* oldInstance = std::exchange(m_scriptInstance, nullptr);

	MemDelete(oldInstance);

	m_scriptInstance = instance;
}

bool Object::TrySetNativeBindingGCHandle(void* value)
{
	if (value == nullptr || m_nativeBindingGCHandle.IsValid())
	{
		return false;
	}

	m_nativeBindingGCHandle = ManagedGCHandle(value);

	if (auto* refCounted = dynamic_cast<RefCounted*>(this))
	{
		refCounted->UpdateNativeBindingGCHandle();
	}

	return true;
}

void Object::ReleaseNativeBindingGCHandle()
{
	if (m_nativeBindingGCHandle.IsValid())
	{
		m_nativeBindingGCHandle.Release();
	}
}

void Object::DetachNativeBindingGCHandle(void* expected)
{
	if (m_nativeBindingGCHandle.GetValue() == expected)
	{
		m_nativeBindingGCHandle.Detach();
	}
}
