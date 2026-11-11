#include "MingEngine/Core/OS/Thread.hpp"

#include <system_error>

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

bool Thread::Start(Callback callback, void* userdata)
{
	if (IsStarted())
	{
		return false;
	}

	if (callback == nullptr)
	{
		return false;
	}

	try
	{
		m_thread = std::thread(callback, userdata);
	}
	catch (std::system_error const&)
	{
		return false;
	}

	return true;
}

void Thread::WaitToFinish()
{
	if (!IsStarted())
	{
		return;
	}

	m_thread.join();
}
