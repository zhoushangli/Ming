#pragma once

#include <thread>

class Thread
{
public:
	using Callback = void (*)(void* userdata);
	using ID       = std::thread::id;

public:
	Thread()  = default;
	~Thread() = default;

	Thread(Thread const&)            = delete;
	Thread& operator=(Thread const&) = delete;
	Thread(Thread&&)                 = delete;
	Thread& operator=(Thread&&)      = delete;

	ID        GetId() const;
	static ID GetCallerId();
	bool      IsStarted() const;

	bool Start(Callback callback, void* userdata);
	void WaitToFinish();

private:
	std::thread m_thread;
};
