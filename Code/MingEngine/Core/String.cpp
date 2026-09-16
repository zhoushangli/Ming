#include "MingEngine/Core/String.hpp"

#include <algorithm>
#include <cstring>
#include <limits>

// The managed mirror Ming.ming_string points at the same code points, so the only
// thing both sides have to agree on is that a String stays one pointer wide.
static_assert(sizeof(String) == sizeof(void*), "String must stay layout compatible with ming_string.");

namespace
{
// This stands for the question mark placeholder
// If we do not know the actual code point, we will use this
constexpr char32_t kReplacementCodePoint = U'\uFFFD';

// Count how many UTF-32 code points a NUL terminated UTF-32 string holds.
// e.g. CodePointCount(U"Ming") returns 4 without reading past the terminator.
uint32_t CodePointCount(char32_t const* data)
{
	uint32_t length = 0;

	while (data[length] != U'\0')
	{
		++length;
	}

	return length;
}

// Decode UTF-8 bytes into UTF-32 code points and return the decoded count.
// e.g. DecodeUtf8("Caf\xC3\xA9", 5, nullptr) returns 4, the buffer variant writes 4 code points.
uint32_t DecodeUtf8(char const* data, size_t byteLength, char32_t* outCodePoints)
{
	uint32_t codePointCount = 0;
	size_t   byteIndex      = 0;

	while (byteIndex < byteLength)
	{
		uint8_t const leadByte  = static_cast<uint8_t>(data[byteIndex]);
		uint32_t      codePoint = kReplacementCodePoint;
		size_t        byteCount = 1;

		// UTF-8 have such rules:
		// 1. 0xxxxxxx - 1 byte
		// 2. 110xxxxx 10xxxxxx - 2 bytes
		// 3. 1110xxxx 10xxxxxx 10xxxxxx - 3 bytes
		// 4. 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx - 4 bytes
		if (leadByte < 0x80)
		{
			codePoint = leadByte;
		}
		else if ((leadByte & 0xE0) == 0xC0)
		{
			codePoint = leadByte & 0x1F;
			byteCount = 2;
		}
		else if ((leadByte & 0xF0) == 0xE0)
		{
			codePoint = leadByte & 0x0F;
			byteCount = 3;
		}
		else if ((leadByte & 0xF8) == 0xF0)
		{
			codePoint = leadByte & 0x07;
			byteCount = 4;
		}

		if (byteCount > 1)
		{
			size_t const remainingByteCount = byteLength - byteIndex;
			size_t       consumedByteCount  = 1;

			// Chech weather the following bytes are valid continuation bytes
			// e.g. 10xxxxxx
			// The first two bits are indicators, the last 6 bits are the actual data
			// 3F == 0011 1111, this is a mask to get the last 6 bits
			while (consumedByteCount < byteCount && consumedByteCount < remainingByteCount
				   && (static_cast<uint8_t>(data[byteIndex + consumedByteCount]) & 0xC0) == 0x80)
			{
				codePoint = (codePoint << 6) | (static_cast<uint8_t>(data[byteIndex + consumedByteCount]) & 0x3F);
				++consumedByteCount;
			}

			if (consumedByteCount != byteCount || codePoint > 0x10FFFF)
			{
				codePoint = kReplacementCodePoint;
			}

			byteCount = consumedByteCount;
		}

		if (outCodePoints != nullptr)
		{
			outCodePoints[codePointCount] = codePoint;
		}

		++codePointCount;
		byteIndex += byteCount;
	}

	return codePointCount;
}

// Encode one code point as UTF-8 bytes into the given buffer.
// e.g. EncodeUtf8(U'\u4e2d', buffer) returns 3 and leaves buffer holding E4 B8 AD.
uint32_t EncodeUtf8(char32_t codePoint, char* outBytes)
{
	if (codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF))
	{
		codePoint = kReplacementCodePoint;
	}

	if (codePoint <= 0x7F)
	{
		outBytes[0] = static_cast<char>(codePoint);
		return 1;
	}

	if (codePoint <= 0x7FF)
	{
		outBytes[0] = static_cast<char>(0xC0 | (codePoint >> 6));
		outBytes[1] = static_cast<char>(0x80 | (codePoint & 0x3F));
		return 2;
	}

	if (codePoint <= 0xFFFF)
	{
		outBytes[0] = static_cast<char>(0xE0 | (codePoint >> 12));
		outBytes[1] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
		outBytes[2] = static_cast<char>(0x80 | (codePoint & 0x3F));
		return 3;
	}

	outBytes[0] = static_cast<char>(0xF0 | (codePoint >> 18));
	outBytes[1] = static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
	outBytes[2] = static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
	outBytes[3] = static_cast<char>(0x80 | (codePoint & 0x3F));
	return 4;
}

// Append the UTF-8 bytes of one code point to the given text.
// e.g. AppendUtf8(U'\u4e2d', text) appends the three bytes E4 B8 AD.
void AppendUtf8(char32_t codePoint, std::string& outText)
{
	char bytes[4] = {};

	outText.append(bytes, EncodeUtf8(codePoint, bytes));
}
} // namespace

String::String(char const* data) : String(data == nullptr ? std::string() : std::string(data)) {}

String::String(std::string const& data)
{
	if (data.empty())
	{
		return;
	}

	// 1) Measure the code points so the shared block is sized exactly
	// 2) Decode the UTF-8 bytes into that block
	uint32_t const codePointCount = DecodeUtf8(data.data(), data.size(), nullptr);

	DecodeUtf8(data.data(), data.size(), m_data.Resize(codePointCount));
}

String::String(char32_t const* data) : String(data, CodePointCount(data == nullptr ? U"" : data)) {}

String::String(char32_t const* data, uint32_t length) { m_data.Assign(data, length); }

String String::operator+(String const& other) const
{
	String result(*this);

	result += other;

	return result;
}

String& String::operator+=(String const& other)
{
	if (other.IsEmpty())
	{
		return *this;
	}

	// 1) Remember the other code points, so appending a string to itself stays valid
	// 2) Grow the block and copy the other code points behind the existing text
	uint32_t const  ownLength   = Length();
	uint32_t const  otherLength = other.Length();
	char32_t const* otherData   = other.Data();

	std::copy_n(otherData, otherLength, m_data.Resize(ownLength + otherLength) + ownLength);

	return *this;
}

String String::operator+(char32_t const* other) const { return *this + String(other); }

String& String::operator+=(char32_t const* other) { return *this += String(other); }

bool String::operator==(String const& other) const
{
	// 1) Compare the counts so the code points can be compared in one pass
	// 2) Compare the two blocks code point by code point
	uint32_t const length = Length();

	if (length != other.Length())
	{
		return false;
	}

	return length == 0 || std::equal(Data(), Data() + length, other.Data());
}

// Compare this string with UTF-8 bytes without allocating a temporary String.
// e.g. EqualsUtf8("Mi", 2) is true for String("Mi") and false for String("Ming").
bool String::EqualsUtf8(char const* data, size_t byteLength) const
{
	// 1) Read the code points once so the comparing loop only touches plain memory
	// 2) Walk both sides and compare the encoded bytes of each code point
	uint32_t const  length     = Length();
	char32_t const* codePoints = Data();

	uint32_t index     = 0;
	size_t   byteIndex = 0;

	while (index < length && byteIndex < byteLength)
	{
		char           bytes[4]  = {};
		uint32_t const byteCount = EncodeUtf8(codePoints[index], bytes);

		if (byteCount > byteLength - byteIndex)
		{
			return false;
		}

		if (std::memcmp(bytes, data + byteIndex, byteCount) != 0)
		{
			return false;
		}

		byteIndex += byteCount;
		++index;
	}

	return index == length && byteIndex == byteLength;
}

bool String::operator==(char const* other) const
{
	if (other == nullptr)
	{
		return IsEmpty();
	}

	return EqualsUtf8(other, std::strlen(other));
}

bool String::operator==(std::string const& other) const { return EqualsUtf8(other.data(), other.size()); }

bool String::Contains(char32_t codePoint) const
{
	// 1) Read the code points once so the loop only touches plain memory
	// 2) Report the first code point that matches the given one
	uint32_t const  length     = Length();
	char32_t const* codePoints = Data();

	for (uint32_t index = 0; index < length; ++index)
	{
		if (codePoints[index] == codePoint)
		{
			return true;
		}
	}

	return false;
}

std::string String::ToUtf8() const
{
	// 1) Read the code points once so the loop only touches plain memory
	// 2) Encode every code point into the UTF-8 result
	uint32_t const  length     = Length();
	char32_t const* codePoints = Data();
	std::string     result;

	result.reserve(length);

	for (uint32_t index = 0; index < length; ++index)
	{
		AppendUtf8(codePoints[index], result);
	}

	return result;
}

String String::Substr(uint32_t start, uint32_t length) const
{
	uint32_t const ownLength = Length();

	if (start >= ownLength || length == 0)
	{
		return String();
	}

	return String(Data() + start, (std::min)(length, ownLength - start));
}

String String::Substr(uint32_t start) const { return Substr(start, Length()); }

String String::FromInt(int64_t value)
{
	// 1) Peel the sign off so the magnitude also holds the smallest negative value
	// 2) Collect the decimal digits from the least significant one
	char32_t digits[24] = {};
	uint32_t digitCount = 0;

	bool const     isNegative = value < 0;
	uint64_t const magnitude  = isNegative ? static_cast<uint64_t>(-(value + 1)) + 1 : static_cast<uint64_t>(value);
	uint64_t       remaining  = magnitude;

	do
	{
		digits[digitCount] = static_cast<char32_t>(U'0' + (remaining % 10));
		++digitCount;
		remaining /= 10;
	} while (remaining != 0);

	if (isNegative)
	{
		digits[digitCount] = U'-';
		++digitCount;
	}

	std::reverse(digits, digits + digitCount);

	return String(digits, digitCount);
}

bool String::TryToInt64(int64_t& outValue) const
{
	// 1) Take an optional sign and require at least one digit after it
	// 2) Accumulate digits while checking that the value still fits
	uint32_t const  length     = Length();
	char32_t const* codePoints = Data();

	if (length == 0)
	{
		return false;
	}

	uint32_t index      = 0;
	bool     isNegative = false;

	if (codePoints[0] == U'-' || codePoints[0] == U'+')
	{
		isNegative = codePoints[0] == U'-';
		index      = 1;
	}

	if (index == length)
	{
		return false;
	}

	uint64_t const limit     = static_cast<uint64_t>((std::numeric_limits<int64_t>::max)()) + (isNegative ? 1U : 0U);
	uint64_t       magnitude = 0;

	for (; index < length; ++index)
	{
		if (codePoints[index] < U'0' || codePoints[index] > U'9')
		{
			return false;
		}

		uint64_t const digit = static_cast<uint64_t>(codePoints[index] - U'0');

		if (magnitude > (limit - digit) / 10)
		{
			return false;
		}

		magnitude = magnitude * 10 + digit;
	}

	// The negative side reaches one value further than the positive one
	if (isNegative && magnitude == limit)
	{
		outValue = (std::numeric_limits<int64_t>::min)();
		return true;
	}

	outValue = isNegative ? -static_cast<int64_t>(magnitude) : static_cast<int64_t>(magnitude);

	return true;
}
