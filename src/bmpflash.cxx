// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <vector>
#include <optional>
#include <string_view>
#include <substrate/indexed_iterator>
#include <substrate/console>
#include <substrate/command_line/arguments>
#include "usbContext.hxx"
#include "bmp.hxx"
#include "options.hxx"
#include "actions.hxx"
#include "version.hxx"

using namespace std::literals::string_view_literals;
using substrate::commandLine::arguments_t;
using substrate::commandLine::flag_t;
using substrate::commandLine::choice_t;
using substrate::commandLine::parseArguments;

arguments_t args{};

[[nodiscard]] auto findBMPs(const usbContext_t &context)
{
	std::vector<usbDevice_t> devices{};
	for (auto device : context.deviceList())
	{
		if (device.vid() == 0x1d50U && device.pid() == 0x6018U)
		{
			console.info("Found BMP at USB address "sv, device.busNumber(), '-', device.portNumber());
			devices.emplace_back(std::move(device));
		}
	}
	return devices;
}

int main(const int argCount, const char *const *const argList)
{
	console = {stdout, stderr};

	// Try to parser the command line arguments
	if (const auto result{parseArguments(static_cast<size_t>(argCount), argList, bmpflash::programOptions)}; !result)
	{
		console.error("Failed to parse command line arguments"sv);
		return 1;
	}
	else // NOLINT(readability-else-after-return)
		args = *result;

	// Handle the version and help options first
	const auto *const version{args["version"sv]};
	const auto *const help{args["help"sv]};
	if (version && help)
	{
		console.error("Can only specify one of --help and --version, not both."sv);
		return 1;
	}
	if (version)
	{
		bmpflash::displayVersion();
		return 0;
	}
	// Display the help if requested or there were no command line options given
	if (help || args.count() == 0U)
	{
		// bmpflash::displayHelp();
		return 0;
	}

	// Try and discover what action the user's requested
	const auto *const actionArg{args["action"sv]};
	if (!actionArg)
	{
		console.error("Action to perform must be specified"sv);
		// bmpflash::displayHelp();
		return 1;
	}
	const auto &action{std::get<choice_t>(*actionArg)};

	// Get a libusb context to perform everything in
	const usbContext_t context{};
	if (!context.valid())
		return 2;

	// Find all BMPs attached to the system
	const auto devices{findBMPs(context)};
	if (devices.empty())
	{
		console.error("Could not find any Black Magic Probes"sv);
		console.warn("Are you sure the permissions on the device are set correctly?"sv);
		return 1;
	}
	// if the user's asked us to dump the info on the attached devices, step into that and exit
	if (action.value() == "info"sv)
		return bmpflash::displayInfo(devices, action.arguments());

	// Filter them to get just one device to work with
	const auto device{bmpflash::filterDevices(devices, std::nullopt)};
	if (!device)
		return 1;

	// Grab the result of trying to run the requested action
	const auto result
	{
		[&]()
		{
			// Dispatch based on the requested action (info's already handled)
			if (action.value() == "sfdp"sv)
				return bmpflash::displaySFDP(*device, action.arguments());
			return false;
		}()
	};

	// Translate the boolean result into a success/fail value and finish up
	return result ? 0 : 1;
}
