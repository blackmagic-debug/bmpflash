// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <string>
#include <string_view>
#include <substrate/conversions>
#include "bmp.hxx"

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;
using substrate::toInt_t;

constexpr static auto remoteResponseOK{'K'};
constexpr static auto remoteResponseParameterError{'P'};
constexpr static auto remoteResponseError{'E'};
constexpr static auto remoteResponseNotSupported{'N'};

constexpr static auto remoteInit{"+#!GA#"sv};
constexpr static auto remoteProtocolVersion{"!HC#"sv};

std::string bmp_t::init() const
{
	// Ask the firmware to initialise its half of remote communications
	writePacket(remoteInit);
	const auto result{readPacket()};
	if (result[0] != remoteResponseOK)
		throw bmpCommsError_t{};
	// Return the firmware version string that pops out from that process
	return result.substr(1U);
}

uint64_t bmp_t::readProtocolVersion() const
{
	// Send a protocol version request packet
	writePacket(remoteProtocolVersion);
	const auto result{readPacket()};
	if (result[0] == remoteResponseNotSupported)
		return 0U;
	if (result[0] != remoteResponseOK)
		throw bmpCommsError_t{};
	const auto versionString{std::string_view{result}.substr(1U)};
	toInt_t<uint64_t> version{versionString.data(), versionString.length() - 1U};
	if (!version.isHex())
		throw std::domain_error{"version value is not a hex number"s};
	return version.fromHex();
}
