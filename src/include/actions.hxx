// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef ACTIONS_HXX
#define ACTIONS_HXX

#include <cstdint>
#include <vector>
#include <optional>
#include <string_view>
#include <substrate/command_line/arguments>
#include "usbDevice.hxx"
#include "bmp.hxx"

namespace bmpflash
{
	using substrate::commandLine::arguments_t;
	using substrate::commandLine::flag_t;
	using substrate::commandLine::choice_t;

	[[nodiscard]] std::optional<usbDevice_t> filterDevices(const std::vector<usbDevice_t> &devices,
		std::optional<std::string_view> deviceSerialNumber) noexcept;
	[[nodiscard]] int32_t displayInfo(const std::vector<usbDevice_t> &devices, const arguments_t &infoArguments);

	[[nodiscard]] bool displaySFDP(const usbDevice_t &device, const arguments_t &sfdpArguments);
	[[nodiscard]] bool provision(const usbDevice_t &device, const arguments_t &provisionArguments);
	[[nodiscard]] bool read(const usbDevice_t &device, const arguments_t &provisionArguments);
	[[nodiscard]] bool write(const usbDevice_t &device, const arguments_t &provisionArguments);
} // namespace bmpflash

#endif /*ACTIONS_HXX*/
