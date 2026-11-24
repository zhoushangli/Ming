#pragma once

#include "MingEngine/Core/Object/Callable.hpp"
#include "MingEngine/Core/OS/Thread.hpp"

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <unordered_map>

// The weakness of our WorkerThreadPool is
// for c# side, when we pass in a lambda, it will create a GCHandle for the lambda
// which basically means hold the instance in c# side
// but we do not have such mechanism in c++ side
class WorkerThreadPool : public Object
{
	MCLASS(WorkerThreadPool, Object)

public:
	using TaskID  = int64_t;
	using GroupID = int64_t;

private:
	// Store the shared data used by multiple Tasks in one group.
	// e.g. Three Tasks share the next index while processing 100 elements.
	struct Group
	{
		Callable m_callback;
		int      m_elementCount   = 0;
		int      m_nextIndex      = 0;
		int      m_remainingTasks = 0;
	};

	// Represent the smallest work unit scheduled and executed by the pool.
	// e.g. A Task invokes one callback or processes indices from its shared Group.
	struct Task
	{
		void (*m_nativeCallback)(void*) = nullptr;
		void* m_userdata = nullptr;
		Callable m_callback;
		Group*   m_group     = nullptr;
		bool     m_completed = false;
	};

public:
	WorkerThreadPool() = default;
	~WorkerThreadPool() override;

	WorkerThreadPool(WorkerThreadPool const&)            = delete;
	WorkerThreadPool& operator=(WorkerThreadPool const&) = delete;
	WorkerThreadPool(WorkerThreadPool&&)                 = delete;
	WorkerThreadPool& operator=(WorkerThreadPool&&)      = delete;

	// Start one pool lifetime; the owner must serialize Start and Stop calls.
	// e.g. pool.Start(3) starts three resident workers and cannot restart after Stop.
	bool Start(int threadCount);

	// Drain accepted work and join workers without reclaiming public task records.
	// e.g. pool.Stop() can be followed by WaitForTaskCompletion(id).
	void Stop();

	// Submit from the owner thread and retain the managed callback until reclamation.
	// e.g. AddTask(callback) runs a Callable created from a C# Action.
	TaskID AddTask(Callable const& callback);
	TaskID AddNativeTask(void (*callback)(void*), void* userdata);
	bool HasManagedTasks() const;
	// Schedule the requested number of Tasks; negative counts use the worker count, empty groups use zero.
	// e.g. AddGroupTask(callback, 100, 5) creates five Tasks sharing 100 elements.
	GroupID AddGroupTask(Callable const& callback, int elementCount, int tasksNeeded = -1);

	// Query completion without reclaiming records; invalid IDs return false.
	// e.g. IsGroupTaskCompleted(id) stays false until every group Task has retired.
	bool IsTaskCompleted(TaskID id) const;
	bool IsGroupTaskCompleted(GroupID id) const;

	// Wait and reclaim once per ID from outside this pool; callbacks must not Wait or Stop.
	// e.g. WaitForTaskCompletion(id) makes id invalid after returning true.
	bool WaitForTaskCompletion(TaskID id);
	bool WaitForGroupTaskCompletion(GroupID id);

private:
	static void BindMethods();

	static void ThreadCallback(void* userdata);
	void        ThreadFunction();
	void        ExecuteTask(Task* task);

public:
	static constexpr TaskID  INVALID_TASK_ID  = -1;
	static constexpr GroupID INVALID_GROUP_ID = -1;

private:
	mutable std::mutex                 m_mutex;
	std::condition_variable            m_workAvailable;
	std::condition_variable            m_taskCompleted;
	std::deque<Task*>                   m_pending;
	std::unordered_map<TaskID, Task*>   m_tasks;
	std::unordered_map<GroupID, Group*> m_groups;
	TaskID                             m_nextId = 0;

	Thread::ID m_ownerThread;
	Thread*    m_threads     = nullptr;
	int        m_threadCount = 0;
	bool       m_accepting   = false;
	bool       m_stopping    = false;
};
