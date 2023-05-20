// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include "crc32.hxx"

namespace bmpflash
{
	// Iterate till we have calculated the CRC
	// NOLINTNEXTLINE(misc-no-recursion)
	constexpr uint32_t calcTableC(const uint32_t poly, const uint32_t byte, const size_t bit) noexcept
		{ return bit ? calcTableC(poly, ((byte & 1U) == 1U ? poly : 0U) ^ (byte >> 1U), bit - 1U) : byte; }
	// Generate CRC for a given byte
	constexpr uint32_t calcTableC(const uint32_t poly, const size_t byte) noexcept
		{ return calcTableC(poly, static_cast<uint32_t>(byte), 8); }

	template<size_t N, uint32_t poly, uint32_t... table> struct calcTable
	{
		constexpr static std::array<const uint32_t, sizeof...(table) + N + 1> value
			{calcTable<N - 1, poly, calcTableC(poly, N), table...>::value};
	};

	template<uint32_t poly, uint32_t... table> struct calcTable<0, poly, table...>
	{
		constexpr static std::array<const uint32_t, sizeof...(table) + 1> value
			{{calcTableC(poly, 0), table...}};
	};

	std::array<const uint32_t, 256> crc32_t::crcTable = calcTable<255, poly>::value;
} // namespace bmpflash
