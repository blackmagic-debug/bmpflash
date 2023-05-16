// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef OPTIONS_HXX
#define OPTIONS_HXX

#include <substrate/console>
#include <substrate/command_line/options>

namespace bmpflash
{
	using namespace std::literals::string_view_literals;
	using substrate::console;
	using namespace substrate::commandLine;

	enum class bus_t
	{
		internal,
		external,
	};

	static inline std::optional<std::any> busSelectionParser(const std::string_view &value) noexcept
	{
		if (value == "int"sv || value == "internal"sv)
			return bus_t::internal;
		if (value == "ext"sv || value == "external"sv)
			return bus_t::external;
		console.error("Invalid value for --bus given, got '"sv, value, "', expecting one of 'int'/'internal' "
			"or 'ext'/'external'"sv);
		return std::nullopt;
	}

	constexpr static auto serialOption
	{
		option_t
		{
			optionFlagPair_t{"-s"sv, "--serial"sv},
			"Use the BMP with the given, possibly partial, matching serial number"sv
		}.takesParameter(optionValueType_t::string)
	};

	constexpr static auto probeOptions{options(serialOption)};

	constexpr static auto deviceOptions
	{
		options
		(
			serialOption,
			option_t
			{
				optionFlagPair_t{"-b"sv, "--bus"sv},
				"Which of the internal (on-board) or external (debug connector attached)\n"
				"busses to use. Specified by giving either 'int' or 'ext'"sv,
			}.takesParameter(optionValueType_t::userDefined, busSelectionParser).required()
		)
	};

	constexpr static auto actions
	{
		optionAlternations
		({
			{
				"info"sv,
				"Display information about attached Black Magic Probes"sv,
				probeOptions,
			},
			{
				"sfdp"sv,
				"Display the SFDP (Serial Flash Discoverable Parameters) information for a Flash chip"sv,
				deviceOptions,
			},
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
