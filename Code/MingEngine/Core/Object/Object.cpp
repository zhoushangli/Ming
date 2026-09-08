#include "MingEngine/Core/Object/Object.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/NativeScript.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/ScriptInstance.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

std::vector<Object*> ObjectDatabase::m_objectSlots;
uint32_t             ObjectDatabase::m_nextObjectUID = 1u;

Object::Object() { ObjectDatabase::AddInstance(this); }
Object::~Object()
{
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

void Object::PostInitialize() { Notification(Notification_PostInitialize, false); }

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

Object::BindMethodsFunc Object::GetBindMethodsFunc() { return &Object::BindMethods; }

void Object::BindMethods()
{
	ClassDatabase::BindMethod("SetScript", &Object::SetScript);
	ClassDatabase::BindMethod("GetScript", &Object::GetScript);
	ClassDatabase::BindMethod("Free", &Object::Free);

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

std::string Object::GetStaticClassName() { return "Object"; }

std::string Object::GetClassName() const { return "Object"; }

ObjectID Object::GetObjectID() const { return m_id; }

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

void Object::Free() { MemDelete(this); }

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

	if (index > ObjectID::kMaxObjectIndex)
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
	m_nextObjectUID    = uid == ObjectID::kMaxObjectUID ? 1u : uid + 1u;

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
		return Variant();
	}

	return Variant(this->m_scriptInstance->GetScript().Get());
}

void Object::SetScript(Variant const& script)
{
	if (m_scriptInstance)
	{
		SetScriptInstance(nullptr);
	}

	Ref<Script> scriptRef = script;
	bool        result    = scriptRef->Instantiate(this);

	if (!result)
	{
		DebuggerPrintf("Failed to instantiate script for Object %s\n", GetClassName().c_str());
		SetScriptInstance(nullptr);
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
	return true;
}

void Object::ReleaseNativeBindingGCHandle()
{
	if (m_nativeBindingGCHandle.IsValid())
	{
		m_nativeBindingGCHandle.Release();
	}
}
