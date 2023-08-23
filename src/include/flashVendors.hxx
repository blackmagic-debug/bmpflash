// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef FLASH_VENDORS_HXX
#define FLASH_VENDORS_HXX

#include <cstdint>
#include <map>
#include <string_view>

using namespace std::literals::string_view_literals;

constexpr inline uint8_t operator ""_u8(const unsigned long long value)
	{ return static_cast<uint8_t>(value); }

static const std::map<uint8_t, std::string_view> flashVendors
{
	{0x1f_u8, "Adesto"sv},
	{0x20_u8, "Numonyx"sv},
	{0xc8_u8, "GigaDevice"sv},
	{0xef_u8, "Winbond"sv},
};

#endif /*FLASH_VENDORS_HXX*/
