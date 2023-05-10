// SPDX-License-Identifier: BSD-3-Clause
#ifndef UTILS_UNITS_HXX
#define UTILS_UNITS_HXX

#include <cstdint>
#include <tuple>
#include <string_view>

namespace bmpflash::utils
{
	using namespace std::literals::string_view_literals;

	inline std::tuple<size_t, std::string_view> humanReadableSize(size_t size)
	{
		if (size < 1024U)
			return {size, "B"sv};
		size /= 1024U;
		if (size < 1024U)
			return {size, "kiB"sv};
		size /= 1024U;
		if (size < 1024U)
			return {size, "MiB"sv};
		size /= 1024U;
		return {size, "GiB"sv};
	}
} // namespace bmpflash::utils

#endif /*INCLUDE_UNITS_HXX*/
