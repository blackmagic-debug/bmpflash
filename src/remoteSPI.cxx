// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <string>
#include <string_view>
#include <fmt/format.h>
#include <substrate/conversions>
#include "bmp.hxx"

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;
using substrate::toInt_t;

constexpr static auto remoteResponseOK{'K'};
constexpr static auto remoteResponseParameterError{'P'};
constexpr static auto remoteResponseError{'E'};
constexpr static auto remoteResponseNotSupported{'N'};

#define REMOTE_UINT8 "{:02x}"

constexpr static auto remoteInit{"+#!GA#"sv};
constexpr static auto remoteProtocolVersion{"!HC#"sv};
constexpr static auto remoteSPIBegin{"!sB" REMOTE_UINT8 "#"sv};
constexpr static auto remoteSPIEnd{"!sE" REMOTE_UINT8 "#"sv};

std::string bmp_t::init() const
{
	// Ask the firmware to initialise its half of remote communications
	writePacket(remoteInit);
	const auto response{readPacket()};
	if (response[0] != remoteResponseOK)
		throw bmpCommsError_t{};
	// Return the firmware version string that pops out from that process
	return response.substr(1U);
}

uint64_t bmp_t::readProtocolVersion() const
{
	// Send a protocol version request packet
	writePacket(remoteProtocolVersion);
	const auto response{readPacket()};
	if (response[0] == remoteResponseNotSupported)
		return 0U;
	if (response[0] != remoteResponseOK)
		throw bmpCommsError_t{};
	const auto versionString{std::string_view{response}.substr(1U)};
	toInt_t<uint64_t> version{versionString.data(), versionString.length() - 1U};
	if (!version.isHex())
		throw std::domain_error{"version value is not a hex number"s};
	return version.fromHex();
}

bool bmp_t::begin(const spiBus_t bus, const spiDevice_t device) noexcept
{
	const auto request{fmt::format(remoteSPIBegin, uint8_t(bus))};
	writePacket(request);
	const auto response{readPacket()};
	if (response[0] == remoteResponseOK)
	{
		spiBus = bus;
		spiDevice = device;
	}
	return response[0] == remoteResponseOK;
}

bool bmp_t::end() noexcept
{
	const auto request{fmt::format(remoteSPIEnd, uint8_t(spiBus))};
	writePacket(request);
	const auto response{readPacket()};
	if (response[0] == remoteResponseOK)
	{
		spiBus = spiBus_t::none;
		spiDevice = spiDevice_t::none;
	}
	return response[0] == remoteResponseOK;
}
