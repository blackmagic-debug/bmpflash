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

	constexpr static auto serialOption
	{
		option_t
		{
			optionFlagPair_t{"-s"sv, "--serial"sv},
			"Use the BMP with the given, possibly partial, matching serial number"sv
		}.takesParameter(optionValueType_t::string)
	};

	constexpr static auto probeOptions{options(serialOption)};

	constexpr static auto actions
	{
		optionAlternations
		({
			{
				"info"sv,
				"Display information about attached Black Magic Probes"sv,
				probeOptions,
			}
		})
	};

	constexpr static auto programOptions
	{
		options
		(
			option_t{optionFlagPair_t{"-h"sv, "--help"sv}, "Display this help message and exit"sv},
			option_t{"--version"sv, "Display the program version information and exit"sv},
			option_t{optionFlagPair_t{"-v"sv, "--verbosity"sv}, "Set the program output verbosity"sv}
				.takesParameter(optionValueType_t::unsignedInt),
			optionSet_t{"action"sv, actions}
		)
	};
} // namespace bmpflash

#endif /*OPTIONS_HXX*/
