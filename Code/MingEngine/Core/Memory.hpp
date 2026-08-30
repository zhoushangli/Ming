#pragma once

#include <utility>

class Object;

// Non-Object allocations do not participate in the Object lifecycle.
inline void PostInitializeHandler(void*) {}

inline bool PreDeleteHandler(void*)
{
	return true;
}

// Here is a little bit tricky
// So the main idea here is to pass the value without copy
// Args&&... args is a universal reference, which just pass the value without copy
// std::forward<Args>(args)... is a perfect forwarding, which will forward the value
// forward means that if the value is an lvalue, it will be forwarded as an lvalue
// if the value is an rvalue, it will be forwarded as an rvalue
template <typename T, typename... Args>
T* MemNew(Args&&... args)
{
	T* object = new T(std::forward<Args>(args)...);
	PostInitializeHandler(object);
	return object;
}

template <typename T>
void MemDelete(T* object)
{
	if (object == nullptr)
	{
		return;
	}

	if (!PreDeleteHandler(object))
	{
		return;
	}

	delete object;
}