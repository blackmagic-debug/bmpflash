// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef UNICODE_HXX
#define UNICODE_HXX

#include <cstddef>
#include <string>
#include <string_view>

namespace utf16
{
	std::string convert(const std::u16string_view &string) noexcept;
	std::u16string convert(const std::string_view &string) noexcept;
} // namespace utf16

#endif /*UNICODE_HXX*/
