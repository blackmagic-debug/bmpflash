// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef FLASH_VENDORS_HXX
#define FLASH_VENDORS_HXX

#include <cstdint>
#include <map>
#include <string_view>

using namespace std::literals::string_view_literals;

static const std::map<uint8_t, std::string_view> flashVendors
{
	{0x1fU, "Adesto"sv},
	{0x20U, "Numonyx"sv},
	{0xc8U, "GigaDevice"sv},
	{0xefU, "Winbond"sv},
};

#endif /*FLASH_VENDORS_HXX*/
