#include "MingEngine/Core/OS/Thread.hpp"

#include <stdexcept>

Thread::ID Thread::GetId() const
{
	return m_thread.get_id();
}

Thread::ID Thread::GetCallerId()
{
	return std::this_thread::get_id();
}

bool Thread::IsStarted() const
{
	return m_thread.joinable();
}

bool Thread::Start(Callback /*callback*/, void* /*userdata*/)
{
	// 1) TODO: Start the callback with its userdata and reject an invalid or unjoined start.
	throw std::logic_error("TODO 1: implement Thread::Start and Thread::WaitToFinish together.");
}

void Thread::WaitToFinish()
{
	if (!IsStarted())
	{
		return;
	}

	// 1) TODO: Wait for the started thread and reclaim its joinable handle.
	throw std::logic_error("TODO 1: implement Thread::WaitToFinish before starting a thread.");
}
