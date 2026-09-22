#pragma once

#include "MingEngine/Core/CowData.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

// Own UTF-32 text inside a shared copy-on-write block, so copying a string only counts one
// more owner instead of copying every code point.
// The managed mirror Ming.ming_string points at the same block, and the block
// header holds the reference count and the code point count the managed side reads.
// e.g. String("Ming") owns 4 UTF-32 code points that its copies share.
class String
{
public:
	String() = default;
	String(char const* data);
	String(std::string const& data);
	String(char32_t const* data);
	String(char32_t const* data, uint32_t length);

	// Copying shares the block and moving takes it over, so both stay allocation free.
	// e.g. String copy = String("Ming") keeps one block with a reference count of 2.
	String(String const& other)                = default;
	String(String&& other) noexcept            = default;
	String& operator=(String const& other)     = default;
	String& operator=(String&& other) noexcept = default;
	~String()                                  = default;

	String  operator+(String const& other) const;
	String& operator+=(String const& other);
	String  operator+(char32_t const* other) const;
	String& operator+=(char32_t const* other);

	uint32_t        Length() const { return m_data.Size(); };
	bool            IsEmpty() const { return m_data.IsEmpty(); };
	char32_t const* Data() const { return m_data.Data(); };

	// Read one code point without a bounds check.
	// e.g. String("Ming")[1] returns U'i'.
	char32_t operator[](uint32_t index) const { return m_data.Data()[index]; };

	bool operator==(String const& other) const;
	bool operator!=(String const& other) const { return !(*this == other); };

	// Compare against UTF-8 text directly, so literals need no temporary String.
	// e.g. node.GetName() == "Camera" walks both sides once without allocating.
	bool operator==(char const* other) const;
	bool operator!=(char const* other) const { return !(*this == other); };
	bool operator==(std::string const& other) const;
	bool operator!=(std::string const& other) const { return !(*this == other); };

	// Report whether the given code point occurs in this string.
	// e.g. String("res://A.png").Contains(U'/') returns true.
	bool Contains(char32_t codePoint) const;

	// Copy a range of code points out of this string, clamped to the available length.
	// e.g. String("Ming").Substr(0, 2) returns "Mi" and Substr(2) returns "ng".
	String Substr(uint32_t start, uint32_t length) const;
	String Substr(uint32_t start) const;

	// Build the decimal text of the given value.
	// e.g. String::FromInt(512) returns "512" and String::FromInt(-7) returns "-7".
	static String FromInt(int64_t value);

	// Convert the UTF-32 content into UTF-8 text for the narrow string APIs.
	// e.g. String(U'\u4e2d').ToUtf8() returns the 3 byte sequence E4 B8 AD.
	std::string ToUtf8() const;

private:
	// The code points live in a shared block whose header stores the reference count and the count.
	CowData<char32_t> m_data;
};

static_assert(sizeof(String) == sizeof(void*), "String must stay one pointer wide for ming_string.");
