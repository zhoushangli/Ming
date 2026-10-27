#include "MingEngine/Editor/EditorUndoRedo.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"

#include <utility>

namespace
{
MethodInfo const* FindMethod(Object const* target, std::string const& name)
{
	ClassInfo const* info = ClassDatabase::GetClassInfo(target->GetClassName());
	while (info != nullptr)
	{
		for (MethodInfo const& method : info->m_methods)
		{
			if (method.m_name == name)
			{
				return &method;
			}
		}

		info = ClassDatabase::GetClassInfo(info->m_parentClassName);
	}

	return nullptr;
}

bool MatchesArgument(Variant const& value, ArgumentInfo const& expected)
{
	if (expected.m_type != Variant::Type::Any && value.GetType() != expected.m_type)
	{
		return false;
	}

	if (expected.m_type == Variant::Type::ObjectPtr && !expected.m_objectClassName.empty())
	{
		Object* object = value.As<Object*>();
		return object == nullptr || ClassDatabase::IsSubclassOf(object->GetClassName(), expected.m_objectClassName);
	}

	return true;
}
}

bool EditorUndoRedo::CreateAction(String const& name, ObjectID historyID, bool backwardUndoOps)
{
	if (m_isReplaying || m_pendingHistoryID.IsValid() || ObjectDatabase::GetInstance(historyID) == nullptr)
	{
		return false;
	}

	m_pendingAction                   = {};
	m_pendingAction.m_name            = name;
	m_pendingAction.m_backwardUndoOps = backwardUndoOps;
	m_pendingHistoryID                = historyID;
	m_pendingValid                    = true;
	return true;
}

bool EditorUndoRedo::AddOperation(
	Object* target, std::string const& name, std::vector<Variant> const& arguments, bool isProperty, bool undo)
{
	if (m_isReplaying || !m_pendingHistoryID.IsValid() || !m_pendingValid)
	{
		return false;
	}

	MethodInfo const*   method = target != nullptr && !isProperty ? FindMethod(target, name) : nullptr;
	PropertyInfo const* property =
		target != nullptr && isProperty ? ClassDatabase::FindProperty(target->GetClassName(), name) : nullptr;
	bool valid = false;
	if (isProperty)
	{
		valid = property != nullptr && property->GetSetter() != nullptr && arguments.size() == 1
				&& (property->m_type == Variant::Type::Any || property->m_type == arguments[0].GetType());
	}
	else if (method != nullptr && method->m_bind != nullptr && method->m_argumentInfos.size() == arguments.size())
	{
		valid = true;
		for (size_t i = 0; i < arguments.size(); ++i)
		{
			if (!MatchesArgument(arguments[i], method->m_argumentInfos[i]))
			{
				valid = false;
				break;
			}
		}
	}

	if (!valid)
	{
		m_pendingValid = false;
		return false;
	}

	Operation operation;
	operation.m_targetID   = target->GetObjectID();
	operation.m_name       = name;
	operation.m_isProperty = isProperty;
	for (Variant const& value : arguments)
	{
		Argument argument;
		argument.m_isObject = value.GetType() == Variant::Type::ObjectPtr;
		if (argument.m_isObject)
		{
			Object* object      = value.As<Object*>();
			argument.m_objectID = object != nullptr ? object->GetObjectID() : ObjectID::Invalid;
		}
		else
		{
			argument.m_value = value;
		}

		operation.m_arguments.push_back(std::move(argument));
	}

	auto& operations = undo ? m_pendingAction.m_undoOps : m_pendingAction.m_doOps;
	operations.push_back(std::move(operation));
	return true;
}

bool EditorUndoRedo::AddDoMethod(Object* target, std::string const& methodName, std::vector<Variant> const& arguments)
{
	return AddOperation(target, methodName, arguments, false, false);
}

bool EditorUndoRedo::AddUndoMethod(Object* target, std::string const& methodName, std::vector<Variant> const& arguments)
{
	return AddOperation(target, methodName, arguments, false, true);
}

bool EditorUndoRedo::AddDoProperty(Object* target, std::string const& propertyName, Variant const& value)
{
	return AddOperation(target, propertyName, { value }, true, false);
}

bool EditorUndoRedo::AddUndoProperty(Object* target, std::string const& propertyName, Variant const& value)
{
	return AddOperation(target, propertyName, { value }, true, true);
}

bool EditorUndoRedo::CommitAction(bool execute)
{
	if (m_isReplaying || !m_pendingHistoryID.IsValid())
	{
		return false;
	}

	ObjectID const historyID = m_pendingHistoryID;
	Action         action    = std::move(m_pendingAction);
	bool const valid   = m_pendingValid && ObjectDatabase::GetInstance(historyID) != nullptr && !action.m_doOps.empty()
						 && !action.m_undoOps.empty();
	m_pendingAction    = {};
	m_pendingHistoryID = ObjectID::Invalid;
	m_pendingValid     = false;
	if (!valid)
	{
		return false;
	}

	History& history = m_histories[historyID];
	history.m_actions.resize(history.m_position);
	history.m_actions.push_back(std::move(action));
	history.m_position = history.m_actions.size();
	if (execute)
	{
		auto operations = history.m_actions.back().m_doOps;
		ReplayOperations(operations);
	}

	return true;
}

void EditorUndoRedo::ReplayOperations(std::vector<Operation> const& operations, bool backwards)
{
	m_isReplaying = true;
	try
	{
		for (size_t i = 0; i < operations.size(); ++i)
		{
			Operation const& operation = operations[backwards ? operations.size() - i - 1 : i];
			Object*          target    = ObjectDatabase::GetInstance(operation.m_targetID);
			if (target == nullptr)
			{
				continue;
			}

			std::vector<Variant> arguments;
			for (Argument const& argument : operation.m_arguments)
			{
				arguments.push_back(
					argument.m_isObject ? Variant(ObjectDatabase::GetInstance(argument.m_objectID)) : argument.m_value);
			}

			if (operation.m_isProperty)
			{
				PropertyInfo const* property = ClassDatabase::FindProperty(target->GetClassName(), operation.m_name);
				if (property != nullptr && property->GetSetter() != nullptr)
				{
					property->GetSetter()->Invoke(target, arguments);
				}
			}
			else
			{
				MethodInfo const* method = FindMethod(target, operation.m_name);
				if (method != nullptr && method->m_bind != nullptr)
				{
					method->m_bind->Invoke(target, arguments);
				}
			}
		}
	}
	catch (...)
	{
		m_isReplaying = false;
		throw;
	}

	m_isReplaying = false;
}

bool EditorUndoRedo::Undo(ObjectID historyID)
{
	if (m_isReplaying || m_pendingHistoryID.IsValid() || !HasUndo(historyID))
	{
		return false;
	}

	History& history = m_histories.at(historyID);
	Action   action  = history.m_actions[--history.m_position];
	ReplayOperations(action.m_undoOps, action.m_backwardUndoOps);
	return true;
}

bool EditorUndoRedo::Redo(ObjectID historyID)
{
	if (m_isReplaying || m_pendingHistoryID.IsValid() || !HasRedo(historyID))
	{
		return false;
	}

	History& history    = m_histories.at(historyID);
	auto     operations = history.m_actions[history.m_position++].m_doOps;
	ReplayOperations(operations);
	return true;
}

bool EditorUndoRedo::HasUndo(ObjectID historyID) const
{
	auto it = m_histories.find(historyID);
	return it != m_histories.end() && it->second.m_position > 0;
}

bool EditorUndoRedo::HasRedo(ObjectID historyID) const
{
	auto it = m_histories.find(historyID);
	return it != m_histories.end() && it->second.m_position < it->second.m_actions.size();
}

void EditorUndoRedo::ClearHistory(ObjectID historyID)
{
	m_histories.erase(historyID);
	if (m_pendingHistoryID == historyID)
	{
		m_pendingAction    = {};
		m_pendingHistoryID = ObjectID::Invalid;
		m_pendingValid     = false;
	}
}

void EditorUndoRedo::ClearAllHistory()
{
	m_histories.clear();
	m_pendingAction    = {};
	m_pendingHistoryID = ObjectID::Invalid;
	m_pendingValid     = false;
}
