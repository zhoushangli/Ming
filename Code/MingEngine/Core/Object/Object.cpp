#include "MingEngine/Core/Object/Object.hpp"

std::vector<Object*> ObjectDatabase::m_objectSlots;
uint32_t             ObjectDatabase::m_nextObjectUID = 1u;

Object::Object() { ObjectDatabase::AddInstance(this); }

Object::Object([[maybe_unused]] Object const& other) : Object() {}

Object& Object::operator=([[maybe_unused]] Object const& other) { return *this; }

Object::Object([[maybe_unused]] Object&& other) : Object() {}

Object& Object::operator=([[maybe_unused]] Object&& other) { return *this; }

Object::~Object() { ObjectDatabase::RemoveInstance(this); }

Object::BindMethodsFunc Object::GetBindMethodsFunc() { return &Object::BindMethods; }

void Object::BindMethods() {}

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

void Object::NotificationForward(int notification) { NotificationForwardV(notification); }

void Object::NotificationBackward(int notification) { NotificationBackwardV(notification); }

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
