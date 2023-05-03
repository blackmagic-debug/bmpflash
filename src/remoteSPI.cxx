// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <string_view>
#include "bmp.hxx"

using namespace std::literals::string_view_literals;

constexpr static auto remoteResponseOK{'K'};
constexpr static auto remoteResponseParameterError{'P'};
constexpr static auto remoteResponseError{'E'};
constexpr static auto remoteResponseNotSupported{'N'};

std::string bmp_t::init() const
{
	// Ask the firmware to initialise its half of remote communications
	writePacket("+#!GS#"sv);
	const auto result{readPacket()};
	if (result[0] != remoteResponseOK)
		throw bmpCommsError_t{};
	return result.substr(1U);
}
