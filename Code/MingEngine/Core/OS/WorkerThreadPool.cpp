#include "MingEngine/Core/OS/WorkerThreadPool.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"

#include <limits>
#include <new>

void WorkerThreadPool::BindMethods()
{
	ClassDatabase::BindMethod("AddTask", &WorkerThreadPool::AddTask, { "callback" });
	ClassDatabase::BindMethod("AddGroupTask", &WorkerThreadPool::AddGroupTask,
		{ "callback", "elementCount", "tasksNeeded" });

	ClassDatabase::BindMethod("IsTaskCompleted", &WorkerThreadPool::IsTaskCompleted, { "id" });
	ClassDatabase::BindMethod("IsGroupTaskCompleted", &WorkerThreadPool::IsGroupTaskCompleted, { "id" });

	ClassDatabase::BindMethod("WaitForTaskCompletion", &WorkerThreadPool::WaitForTaskCompletion, { "id" });
	ClassDatabase::BindMethod("WaitForGroupTaskCompletion", &WorkerThreadPool::WaitForGroupTaskCompletion, { "id" });
}

WorkerThreadPool::~WorkerThreadPool()
{
	Stop();

	// 1) Release records that their callers did not reclaim before pool destruction.
	for (auto const& entry : m_tasks)
	{
		delete entry.second;
	}

	for (auto const& entry : m_groups)
	{
		delete entry.second;
	}

	delete[] m_threads;
}

bool WorkerThreadPool::Start(int threadCount)
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_threads || threadCount <= 0)
		{
			return false;
		}

		// 1) Allocate stable storage before starting any worker.
		m_threads = new (std::nothrow) Thread[threadCount];
		if (m_threads == nullptr)
		{
			return false;
		}

		m_ownerThread = Thread::GetCallerId();
		m_threadCount = threadCount;
		int started = 0;
		for (; started < threadCount; ++started)
		{
			if (!m_threads[started].Start(ThreadCallback, this))
			{
				break;
			}
		}

		if (started == threadCount)
		{
			m_accepting = true;
			return true;
		}
	}

	// 2) Release the state lock before joining partially started workers.
	Stop();
	return false;
}

void WorkerThreadPool::Stop()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_threads || m_stopping)
		{
			return;
		}

		// 1) Reject submissions while allowing workers to drain accepted Tasks.
		m_accepting = false;
		m_stopping  = true;
	}

	// 2) Wake idle workers and join without holding the queue lock.
	m_workAvailable.notify_all();
	for (int index = 0; index < m_threadCount; ++index)
	{
		m_threads[index].WaitToFinish();
	}
}

WorkerThreadPool::TaskID WorkerThreadPool::AddTask(Callable const& callback)
{
	if (Thread::GetCallerId() != m_ownerThread || !callback.CanRunOnWorker(0))
	{
		return INVALID_TASK_ID;
	}

	TaskID id = INVALID_TASK_ID;
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (!m_accepting || m_nextId == std::numeric_limits<TaskID>::max())
		{
			return INVALID_TASK_ID;
		}

		id               = m_nextId++;
		Task* task = nullptr;
		try
		{
			task = new Task();
			task->m_callback = callback;
			m_tasks.emplace(id, task);
			m_pending.push_back(task);
		}
		catch (...)
		{
			m_tasks.erase(id);
			lock.unlock();
			delete task;
			return INVALID_TASK_ID;
		}
	}

	m_workAvailable.notify_one();
	return id;
}

WorkerThreadPool::TaskID WorkerThreadPool::AddNativeTask(void (*callback)(void*), void* userdata)
{
	if (Thread::GetCallerId() != m_ownerThread || callback == nullptr)
	{
		return INVALID_TASK_ID;
	}

	std::unique_lock<std::mutex> lock(m_mutex);
	if (!m_accepting || m_nextId == std::numeric_limits<TaskID>::max())
	{
		return INVALID_TASK_ID;
	}

	TaskID const id = m_nextId++;
	Task* task = new (std::nothrow) Task();
	if (task == nullptr)
	{
		return INVALID_TASK_ID;
	}

	task->m_nativeCallback = callback;
	task->m_userdata = userdata;
	try
	{
		m_tasks.emplace(id, task);
		m_pending.push_back(task);
	}
	catch (...)
	{
		m_tasks.erase(id);
		delete task;
		return INVALID_TASK_ID;
	}

	lock.unlock();
	m_workAvailable.notify_one();
	return id;
}

bool WorkerThreadPool::HasManagedTasks() const
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto const& entry : m_tasks)
	{
		if (entry.second->m_nativeCallback == nullptr)
		{
			return true;
		}
	}
	return !m_groups.empty();
}

WorkerThreadPool::GroupID WorkerThreadPool::AddGroupTask(Callable const& callback, int elementCount, int tasksNeeded)
{
	if (Thread::GetCallerId() != m_ownerThread || !callback.CanRunOnWorker(1)
		|| elementCount < 0 || (elementCount > 0 && tasksNeeded == 0))
	{
		return INVALID_GROUP_ID;
	}

	GroupID id = INVALID_GROUP_ID;
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		if (!m_accepting || m_nextId == std::numeric_limits<TaskID>::max())
		{
			return INVALID_GROUP_ID;
		}

		if (tasksNeeded < 0)
		{
			tasksNeeded = m_threadCount;
		}

		if (elementCount == 0)
		{
			tasksNeeded = 0;
		}

		// 1) Publish the entire group while holding the queue lock.
		Group* group = nullptr;
		size_t const previousPendingCount = m_pending.size();
		id = m_nextId++;
		try
		{
			group = new Group();
			group->m_callback       = callback;
			group->m_elementCount   = elementCount;
			group->m_remainingTasks = tasksNeeded;
			m_groups.emplace(id, group);

			for (int index = 0; index < tasksNeeded; ++index)
			{
				m_pending.push_back(nullptr);
				Task* task = new Task();
				task->m_group = group;
				m_pending.back() = task;
			}
		}
		catch (...)
		{
			// 2) Undo partial publication before a worker can take a Task.
			while (m_pending.size() > previousPendingCount)
			{
				delete m_pending.back();
				m_pending.pop_back();
			}

			m_groups.erase(id);
			lock.unlock();
			delete group;
			return INVALID_GROUP_ID;
		}
	}

	m_workAvailable.notify_all();
	return id;
}

bool WorkerThreadPool::IsTaskCompleted(TaskID id) const
{
	if (Thread::GetCallerId() != m_ownerThread)
	{
		return false;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	auto const                  found = m_tasks.find(id);
	return found != m_tasks.end() && found->second->m_completed;
}

bool WorkerThreadPool::IsGroupTaskCompleted(GroupID id) const
{
	if (Thread::GetCallerId() != m_ownerThread)
	{
		return false;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	auto const                  found = m_groups.find(id);
	return found != m_groups.end() && found->second->m_remainingTasks == 0;
}

bool WorkerThreadPool::WaitForTaskCompletion(TaskID id)
{
	if (Thread::GetCallerId() != m_ownerThread)
	{
		return false;
	}

	std::unique_lock<std::mutex> lock(m_mutex);
	auto const                   found = m_tasks.find(id);
	if (found == m_tasks.end())
	{
		return false;
	}

	// 1) Release the lock while waiting; the table keeps the Task address stable.
	Task* task = found->second;
	m_taskCompleted.wait(
		lock,
		[task]
		{
			return task->m_completed;
		});

	// 2) Reclaim only after the worker's final access, destroying captures outside the lock.
	m_tasks.erase(id);
	lock.unlock();
	delete task;
	return true;
}

bool WorkerThreadPool::WaitForGroupTaskCompletion(GroupID id)
{
	if (Thread::GetCallerId() != m_ownerThread)
	{
		return false;
	}

	std::unique_lock<std::mutex> lock(m_mutex);
	auto const                   found = m_groups.find(id);
	if (found == m_groups.end())
	{
		return false;
	}

	// 1) Keep the shared data alive until every Task has finished and been destroyed.
	Group* group = found->second;
	m_taskCompleted.wait(
		lock,
		[group]
		{
			return group->m_remainingTasks == 0;
		});

	// 2) No Task can access this Group after the final retirement.
	m_groups.erase(id);
	lock.unlock();
	delete group;
	return true;
}

void WorkerThreadPool::ThreadCallback(void* userdata)
{
	static_cast<WorkerThreadPool*>(userdata)->ThreadFunction();
}

void WorkerThreadPool::ThreadFunction()
{
	while (true)
	{
		Task* task = nullptr;
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_workAvailable.wait(
				lock,
				[this]
				{
					return m_stopping || !m_pending.empty();
				});

			if (m_stopping && m_pending.empty())
			{
				return;
			}

			task = m_pending.front();
			m_pending.pop_front();
		}

		// 1) Invoke user callbacks outside the queue and state lock.
		ExecuteTask(task);
	}
}

void WorkerThreadPool::ExecuteTask(Task* task)
{
	Group* group = task->m_group;
	if (group == nullptr)
	{
		if (task->m_nativeCallback != nullptr)
		{
			task->m_nativeCallback(task->m_userdata);
		}
		else
		{
			task->m_callback.InvokeOnWorker(nullptr);
		}
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			task->m_completed = true;
		}

		m_taskCompleted.notify_all();
		return;
	}

	// 1) Claim unique indices under the lock and calculate outside it.
	while (true)
	{
		int index = 0;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (group->m_nextIndex >= group->m_elementCount)
			{
				break;
			}

			index = group->m_nextIndex++;
		}

		Variant const argument(index);
		group->m_callback.InvokeOnWorker(&argument);
	}

	// 2) Destroy this internal Task before retiring its use of the shared Group.
	delete task;
	bool completed = false;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		completed = --group->m_remainingTasks == 0;
	}

	// 3) The final Task publishes completion without accessing the Group again.
	if (completed)
	{
		m_taskCompleted.notify_all();
	}
}
