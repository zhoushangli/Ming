#include "MingEngine/Core/Object/Signal.hpp"

#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

Signal::Signal(Object const* object, std::string const& name) : m_name(name)
{
	if (object != nullptr)
	{
		m_objectID = object->GetObjectID();
	}
}

Object* Signal::GetObject() const
{
	return ObjectDatabase::GetInstance(m_objectID);
}

bool Signal::operator==(Signal const& other) const
{
	return m_objectID == other.m_objectID && m_name == other.m_name;
}

bool Signal::operator!=(Signal const& other) const
{
	return !(*this == other);
}

bool Signal::Connect(Callable const& callable) const
{
	Object* object = GetObject();
	return object != nullptr && object->Connect(m_name, callable);
}

bool Signal::Disconnect(Callable const& callable) const
{
	Object* object = GetObject();
	return object != nullptr && object->Disconnect(m_name, callable);
}

bool Signal::IsConnected(Callable const& callable) const
{
	Object* object = GetObject();
	return object != nullptr && object->IsConnected(m_name, callable);
}

bool Signal::HasConnections() const
{
	Object* object = GetObject();
	return object != nullptr && object->HasSignalConnections(m_name);
}

bool Signal::Emit() const
{
	return Emit({});
}

bool Signal::Emit(std::vector<Variant> const& arguments) const
{
	Object* object = GetObject();
	return object != nullptr && object->EmitSignal(m_name, arguments);
}
