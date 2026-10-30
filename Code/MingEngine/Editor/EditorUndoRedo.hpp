#pragma once

#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

#include <map>
#include <string>
#include <vector>

class EditorUndoRedo
{
private:
	struct Argument
	{
		Variant  m_value;
		ObjectID m_objectID = ObjectID::Invalid;
		bool     m_isObject = false;
	};

	struct Operation
	{
		ObjectID              m_targetID = ObjectID::Invalid;
		std::string           m_name;
		std::vector<Argument> m_arguments;
		bool                  m_isProperty = false;
	};

	struct Action
	{
		String                 m_name;
		std::vector<Operation> m_doOps;
		std::vector<Operation> m_undoOps;
		bool                   m_backwardUndoOps = false;
	};

	struct History
	{
		std::vector<Action> m_actions;
		size_t              m_position = 0;
	};

public:
	bool HasUndo(ObjectID historyID) const;
	bool HasRedo(ObjectID historyID) const;

	// Group do/undo operations in the history belonging to the given object.
	// e.g. CreateAction("Edit Text", lineEdit->GetObjectID()) keeps each field's history separate.
	bool CreateAction(String const& name, ObjectID historyID, bool backwardUndoOps = false);

	bool AddDoMethod(Object* target, std::string const& methodName, std::vector<Variant> const& arguments = {});
	bool AddUndoMethod(Object* target, std::string const& methodName, std::vector<Variant> const& arguments = {});
	bool AddDoProperty(Object* target, std::string const& propertyName, Variant const& value);
	bool AddUndoProperty(Object* target, std::string const& propertyName, Variant const& value);

	// Record the pending action and optionally execute its do operations for the first time.
	// e.g. CommitAction(false) records an edit that has already been applied.
	bool CommitAction(bool execute = true);

	bool Undo(ObjectID historyID);
	bool Redo(ObjectID historyID);

	void ClearHistory(ObjectID historyID);
	void ClearAllHistory();

private:
	bool AddOperation(
		Object* target, std::string const& name, std::vector<Variant> const& arguments, bool isProperty, bool undo);

	void ReplayOperations(std::vector<Operation> const& operations, bool backwards = false);

private:
	std::map<ObjectID, History> m_histories;
	Action                      m_pendingAction;
	ObjectID                    m_pendingHistoryID = ObjectID::Invalid;
	bool                        m_pendingValid     = false;
	bool                        m_isReplaying      = false;
};
