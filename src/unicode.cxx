// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <cstring>
#include <limits>
#include <type_traits>
#include "unicode.hxx"

template<typename char_t, typename uint_t = std::make_unsigned_t<char_t>>
	inline auto safeIndex(const char_t *const string, const size_t index, const size_t length) noexcept
{
	if (index >= length)
		return std::numeric_limits<uint_t>::max();
	uint_t result{};
	std::memcpy(&result, string + index, sizeof(char_t));
	return result;
}

size_t countUnits(const std::u16string_view &string) noexcept
{
	const auto *const stringData{string.data()};
	const auto length{string.length()};
	size_t count{0U};
	for (size_t idx = 0; idx < length; ++idx)
	{
		// Get the first code unit of the sequence
		const auto unitA{safeIndex(stringData, idx, length)};
		// Check for it being a surrogate pair high unit
		if ((unitA & 0xfe00U) == 0xd800U)
		{
			++idx;
			// Get the second code unit of the pair
			const auto unitB{safeIndex(stringData, idx, length)};
			// And validate that it's a surrogate pair low unit
			if ((unitB & 0xfe00U) != 0xdc00U)
				return 0U;
			// Surrogate pairs encode things above U+010000, which are all 4-byte in UTF-8
			count += 4U;
		}
		// Check for it being a lone surrogate pair low unit
		else if ((unitA & 0xfe00U) == 0xdc00U)
			return 0U;
		// Something in the Basic Multilingual Plane
		else
		{
			// If it's also above 0x07ff, that's a 3-byte unit
			if (unitA > 0x07ffU)
				count += 3U;
			// If the value is more than 0x007f, that's a 2-byte unit
			else if (unitA > 0x007fU)
				count += 2U;
			else if (unitA != 0U)
			// Otherwise it's a single byte unit
				++count;
			// If we found a NUL character, stop counting.
			if (unitA == 0U)
				break;
		}
	}
	return count;
}

std::string utf16::convert(const std::u16string_view &string) noexcept
{
	const auto lengthUTF8{countUnits(string)};
	// Check if the string is well formed or not for the conversion
	if (!lengthUTF8)
		return {};
	// Allocate a suitably long string
	std::string result(lengthUTF8, '\0');
	// Prepare to iterate over the input string data
	const auto *const stringData{string.data()};
	const auto lengthUTF16{string.length()};
	for (size_t inputOffset{0}, outputOffset{0}; inputOffset < lengthUTF16; ++inputOffset)
	{
		// Read a code unit to convert
		const auto unitA{safeIndex(stringData, inputOffset, lengthUTF16)};
		// If it's a surrogate pair
		if ((unitA & 0xfe00U) == 0xd800U)
		{
			// Recover the upper 10 (11) bits from the high surrogate unit we already have
			const auto upper{(unitA & 0x03ffU) + 0x0040U};
			++inputOffset;
			// Recover the lower 10 bits from the low surrogate unit
			const auto lower{safeIndex(stringData, inputOffset, lengthUTF16) & 0x03ffU};

			// Rebuild the character as a 4 byte UTF-8 sequence
			result[outputOffset + 0] = static_cast<char>(0xf0U | (uint8_t(upper >> 8U) & 0x07U));
			result[outputOffset + 1] = static_cast<char>(0x80U | (uint8_t(upper >> 2U) & 0xefU));
			result[outputOffset + 2] = static_cast<char>(0x80U | (uint8_t(upper << 4U) & 0x30U) |
				(uint8_t(lower >> 6U) & 0x0fU));
			result[outputOffset + 3] = static_cast<char>(0x80U | uint8_t(lower & 0x3fU));
			outputOffset += 4U;
		}
		else
		{
			// If the character's more than 0x07ff, rebuild it as a 3 byte UTF-8 sequence
			if (unitA > 0x07ffU)
			{
				result[outputOffset + 0] = static_cast<char>(0xe0U | (uint8_t(unitA >> 12U) & 0x0fU));
				result[outputOffset + 1] = static_cast<char>(0x80U | (uint8_t(unitA >> 6U) & 0x3fU));
				result[outputOffset + 2] = static_cast<char>(0x80U | uint8_t(unitA & 0x3fU));
				outputOffset += 3U;
			}
			// Else if it's more than 0x007f, rebuild it as a 2 byte UTF-8 sequence
			else if (unitA > 0x007fU)
			{
				result[outputOffset + 0] = static_cast<char>(0xc0U | (uint8_t(unitA >> 6U) & 0x1fU));
				result[outputOffset + 1] = static_cast<char>(0x80U | uint8_t(unitA & 0x3fU));
				outputOffset += 2U;
			}
			// Finally, handle single byte values
			else
			{
				result[outputOffset] = static_cast<char>(unitA);
				++outputOffset;
			}
		}
	}
	return result;
}
