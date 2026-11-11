#pragma once

#include "MingEngine/Core/Object/Variant.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

struct CallError
{
public:
	enum class Error : int32_t
	{
		Ok = 0,
		InvalidTarget = 1,
		InvalidMethod = 2,
		TooFewArguments = 3,
		TooManyArguments = 4,
		InvalidArgument = 5,
		ExecutionFailed = 6,
	};

public:
	Error         m_error                 = Error::Ok;
	int32_t       m_argument              = -1;
	int32_t       m_expectedArgumentCount = 0;
	Variant::Type m_expectedType          = Variant::Type::Empty;
};

static_assert(std::is_standard_layout_v<CallError> && sizeof(CallError) == 16);
static_assert(offsetof(CallError, m_error) == 0 && offsetof(CallError, m_argument) == 4);
static_assert(offsetof(CallError, m_expectedArgumentCount) == 8 && offsetof(CallError, m_expectedType) == 12);
