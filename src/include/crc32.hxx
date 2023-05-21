// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef CRC32_HXX
#define CRC32_HXX

#include <cstdint>
#include <cstddef>
#include <array>
#include <type_traits>
#include <substrate/span>

namespace bmpflash
{
	constexpr uint32_t calcPolynomial(const size_t bit) noexcept { return 1U << (31U - bit); }
	template<typename T, typename... U> constexpr std::enable_if_t<sizeof...(U) != 0, uint32_t>
		calcPolynomial(T bit, U ...bits) noexcept { return calcPolynomial(bit) | calcPolynomial(bits...); }

	struct crc32_t final
	{
	private:
		constexpr static uint32_t poly = calcPolynomial(0U, 1U, 2U, 4U, 5U, 7U, 8U, 10U, 11U, 12U, 16U, 22U, 23U, 26U);
		static_assert(poly == UINT32_C(0xedb88320), "Polynomial calculation failure");
		static std::array<const uint32_t, 256> crcTable;

	public:
		static void crc(uint32_t &crc, const substrate::span<const uint8_t> data) noexcept
		{
			crc ^= UINT32_C(0xffffffff);
			for (const auto &item : data)
				crc = crcTable[uint8_t(crc ^ item)] ^ (crc >> 8U);
			crc ^= UINT32_C(0xffffffff);
		}

		crc32_t() = delete;
	};
} // namespace bmpflash

#endif /*CRC32_HXX*/
