#pragma once

#include <cstdint>
#include <string>

// Own a UTF-32 text buffer. The managed mirror Ming.InteropTypes.ming_string keeps this
// exact layout, so both sides can read the same string memory without extra callbacks.
// e.g. String("Ming") owns 4 UTF-32 code points and frees them in ~String.
class String
{
public:
	String() = default;
	String(char const* data);
	String(std::string const& data);
	String(char32_t const* data);
	String(char32_t const* data, uint32_t length);

	String(String const& other);
	String(String&& other) noexcept;

	String& operator=(String const& other);
	String& operator=(String&& other) noexcept;
	String  operator+(String const& other) const;
	String& operator+=(String const& other);
	String  operator+(char32_t const* other) const;
	String& operator+=(char32_t const* other);

	~String();

	uint32_t        Length() const { return m_length; };
	bool            IsEmpty() const { return m_length == 0; };
	char32_t const* Data() const { return m_data; };

	// Read one code point without a bounds check.
	// e.g. String("Ming")[1] returns U'i'.
	char32_t operator[](uint32_t index) const { return m_data[index]; };

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

	// Parse this string as a decimal integer and report whether it is one.
	// e.g. TryToInt64("12", value) returns true with value 12; "1x" returns false.
	bool TryToInt64(int64_t& outValue) const;

	// Convert the UTF-32 content into UTF-8 text for the narrow string APIs.
	// e.g. String(U'\u4e2d').ToUtf8() returns the 3 byte sequence E4 B8 AD.
	std::string ToUtf8() const;

private:
	// Compare this string with UTF-8 bytes without allocating a temporary String.
	// e.g. EqualsUtf8("Mi", 2) is true for String("Mi") and false for String("Ming").
	bool EqualsUtf8(char const* data, size_t byteLength) const;

private:
	char32_t* m_data   = nullptr;
	uint32_t  m_length = 0;
};