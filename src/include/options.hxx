// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef OPTIONS_HXX
#define OPTIONS_HXX

#include <substrate/console>
#include <substrate/command_line/options>
#include "bmp.hxx"

namespace bmpflash
{
	using namespace std::literals::string_view_literals;
	using substrate::console;
	using namespace substrate::commandLine;

	static inline std::optional<std::any> busSelectionParser(const std::string_view &value) noexcept
	{
		if (value == "int"sv || value == "internal"sv)
			return spiBus_t::internal;
		if (value == "ext"sv || value == "external"sv)
			return spiBus_t::external;
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

	constexpr static auto fileOption
	{
		option_t
		{
			optionValue_t{"fileName"sv},
			"Use the given file name (including path relative to your working directory) for the operation"sv
		}.takesParameter(optionValueType_t::path).required()
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

	constexpr static auto provisioningOptions{options(serialOption, fileOption)};
	constexpr static auto generalFlashOptions{options(deviceOptions, fileOption)};

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
			{
				"provision"sv,
				"Provision a BMP's on-board Flash for use with the auto-programming command in standalone mode"sv,
				provisioningOptions,
			},
			{
				"read"sv,
				"Read the contents of a Flash chip into the file specified"sv,
				generalFlashOptions,
			},
			{
				"write"sv,
				"Write the contents of the file specified into a Flash chip"sv,
				generalFlashOptions,
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
				.takesParameter(optionValueType_t::unsignedInt).valueRange(0U, 1U),
			optionSet_t{"action"sv, actions}
		)
	};
} // namespace bmpflash

#endif /*OPTIONS_HXX*/
