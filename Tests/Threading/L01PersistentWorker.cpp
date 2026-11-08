#include "MingEngine/Core/OS/Thread.hpp"

#include <condition_variable>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

class PersistentWorker
{
public:
	struct Result
	{
		int        m_input    = 0;
		int64_t    m_value    = 0;
		Thread::ID m_threadId;
	};

public:
	PersistentWorker() = default;
	~PersistentWorker();

	PersistentWorker(PersistentWorker const&)            = delete;
	PersistentWorker& operator=(PersistentWorker const&) = delete;
	PersistentWorker(PersistentWorker&&)                 = delete;
	PersistentWorker& operator=(PersistentWorker&&)      = delete;

	// Read completed results on the owner thread after Stop has returned.
	// e.g. worker.Stop(); auto const& results = worker.GetResults();
	std::vector<Result> const& GetResults() const;

	bool Start();
	bool Submit(int input);
	void Stop();

private:
	static void ThreadCallback(void* userdata);
	void        ThreadFunction();

private:
	std::mutex              m_mutex;
	std::condition_variable m_condition;
	std::queue<int>         m_tasks;
	bool                    m_stopping = false;

	std::vector<Result> m_results;
	Thread              m_thread;
};

PersistentWorker::~PersistentWorker()
{
	Stop();
}

std::vector<PersistentWorker::Result> const& PersistentWorker::GetResults() const
{
	return m_results;
}

bool PersistentWorker::Start()
{
	// 2) TODO: Start ThreadCallback with this instance as userdata.
	throw std::logic_error("TODO 2: implement PersistentWorker::Start.");
}

bool PersistentWorker::Submit(int /*input*/)
{
	// 3) TODO: Accept one input under the queue lock, then notify the worker.
	throw std::logic_error("TODO 3: implement PersistentWorker::Submit.");
}

void PersistentWorker::Stop()
{
	if (!m_thread.IsStarted())
	{
		return;
	}

	// 5) TODO: Stop accepting tasks, wake the worker, and join outside the queue lock.
	throw std::logic_error("TODO 5: implement PersistentWorker::Stop before starting the worker.");
}

void PersistentWorker::ThreadCallback(void* userdata)
{
	static_cast<PersistentWorker*>(userdata)->ThreadFunction();
}

void PersistentWorker::ThreadFunction()
{
	// 2) TODO: Block while idle, and wake when work or a shutdown request arrives.
	// 3) TODO: Take one accepted task under the queue lock.
	// 4) TODO: Compute its square outside the lock and record its result.
	// 5) TODO: Exit only after stopping has been requested and all accepted tasks are finished.
}

static void RecordThreadId(void* userdata)
{
	*static_cast<Thread::ID*>(userdata) = Thread::GetCallerId();
}

int main(int argc, char** argv)
{
	std::string const mode = argc == 2 ? argv[1] : "--tasks";
	if (argc > 2 || (mode != "--thread" && mode != "--empty" && mode != "--tasks"))
	{
		std::cerr << "Usage: ThreadingLesson.exe [--thread|--empty|--tasks]\n";
		return 1;
	}

	try
	{
		auto const ownerId = Thread::GetCallerId();
		if (mode == "--thread")
		{
			Thread thread;
			Thread::ID callbackId;
			if (!thread.Start(&RecordThreadId, &callbackId))
			{
				throw std::runtime_error("Thread failed to start.");
			}

			auto const startedId = thread.GetId();
			bool const started = thread.IsStarted();
			thread.WaitToFinish();
			if (!started || thread.IsStarted() || callbackId == Thread::ID{}
				|| callbackId == ownerId || callbackId != startedId)
			{
				throw std::runtime_error("Thread identity or join lifecycle is incorrect.");
			}

			std::cout << "[PASS] Thread startup, identity, and join\n";
			return 0;
		}

		bool const empty = mode == "--empty";
		PersistentWorker worker;

		if (!worker.Start())
		{
			throw std::runtime_error("Worker failed to start.");
		}

		std::vector<int> const inputs = empty ? std::vector<int>{} : std::vector<int>{3, 5, 8};
		for (int input : inputs)
		{
			if (!worker.Submit(input))
			{
				throw std::runtime_error("Worker rejected a task before shutdown.");
			}
		}

		worker.Stop();

		auto const& results = worker.GetResults();
		if (results.size() != inputs.size())
		{
			throw std::runtime_error("Shutdown did not drain every accepted task.");
		}

		for (size_t index = 0; index < results.size(); ++index)
		{
			auto const& result = results[index];
			int64_t const expected = static_cast<int64_t>(inputs[index]) * inputs[index];
			if (result.m_input != inputs[index] || result.m_value != expected)
			{
				throw std::runtime_error("A result is incorrect or out of submission order.");
			}

			if (result.m_threadId == Thread::ID{} || result.m_threadId == ownerId
				|| result.m_threadId != results.front().m_threadId)
			{
				throw std::runtime_error("Tasks did not run on one distinct worker thread.");
			}

			std::cout << result.m_input << " -> " << result.m_value << " (thread " << result.m_threadId << ")\n";
		}

		if (worker.Submit(10))
		{
			throw std::runtime_error("Worker accepted a task after shutdown.");
		}

		std::cout << "[PASS] " << (empty ? "Empty shutdown" : "Task results and draining shutdown") << '\n';
		return 0;
	}
	catch (std::logic_error const& error)
	{
		std::cerr << "[INCOMPLETE] " << error.what() << '\n';
		return 2;
	}
	catch (std::exception const& error)
	{
		std::cerr << "[FAIL] " << error.what() << '\n';
		return 1;
	}
}
