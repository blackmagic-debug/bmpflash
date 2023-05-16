// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef OPTIONS_HXX
#define OPTIONS_HXX

#include <substrate/command_line/options>

namespace bmpflash
{
	using namespace std::literals::string_view_literals;
	using namespace substrate::commandLine;

	constexpr static auto programOptions
	{
		options
		(
			option_t{optionFlagPair_t{"-h"sv, "--help"sv}, "Display this help message and exit"sv},
			option_t{"--version"sv, "Display the program version information and exit"sv},
			option_t{optionFlagPair_t{"-v"sv, "--verbosity"sv}, "Set the program output verbosity"sv}
				.takesParameter(optionValueType_t::unsignedInt)
		)
	};
} // namespace bmpflash

#endif /*OPTIONS_HXX*/
