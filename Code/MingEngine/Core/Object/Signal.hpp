#pragma once

#include "MingEngine/Core/Object/ObjectID.hpp"

#include <string>
#include <vector>

class Callable;
class Object;
class Variant;

// Identify an object's signal without owning the object or its connections.
// e.g. Signal(button, "pressed") accesses the connections stored by button.
class Signal
{
public:
	Signal() = default;
	Signal(Object const* object, std::string const& name);

	Object*            GetObject() const;
	ObjectID           GetObjectID() const { return m_objectID; }
	std::string const& GetName() const { return m_name; }

	bool operator==(Signal const& other) const;
	bool operator!=(Signal const& other) const;

	bool Connect(Callable const& callable) const;
	bool Disconnect(Callable const& callable) const;
	bool IsConnected(Callable const& callable) const;
	bool HasConnections() const;

	bool Emit() const;
	bool Emit(std::vector<Variant> const& arguments) const;

private:
	ObjectID    m_objectID = ObjectID::Invalid;
	std::string m_name;
};
