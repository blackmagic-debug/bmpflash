// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <string>
#include <string_view>
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4061 4365)
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif
#include <fmt/format.h>
#if defined(_MSC_VER)
#pragma warning(pop)
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#include <substrate/conversions>
#include <substrate/span>
#include <substrate/index_sequence>
#include "bmp.hxx"

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;
using substrate::toInt_t;
using substrate::fromInt;
using substrate::indexSequence_t;

constexpr static auto remoteResponseOK{'K'};
constexpr static auto remoteResponseParameterError{'P'};
constexpr static auto remoteResponseError{'E'};
constexpr static auto remoteResponseNotSupported{'N'};

#define REMOTE_UINT8  "{:02x}"
#define REMOTE_UINT16 "{:04x}"
#define REMOTE_UINT24 "{:06x}"

constexpr static auto remoteInit{"+#!GA#"sv};
constexpr static auto remoteProtocolVersion{"!HC#"sv};
constexpr static auto remoteSPIBegin{"!sB" REMOTE_UINT8 "#"sv};
constexpr static auto remoteSPIEnd{"!sE" REMOTE_UINT8 "#"sv};
constexpr static auto remoteSPIChipID{"!sI" REMOTE_UINT8 REMOTE_UINT8 "#"sv};
constexpr static auto remoteSPIRead{"!sr" REMOTE_UINT8 REMOTE_UINT8 REMOTE_UINT16 REMOTE_UINT24 REMOTE_UINT16 "#"sv};
constexpr static auto remoteSPIWrite{"!sw" REMOTE_UINT8 REMOTE_UINT8 REMOTE_UINT16 REMOTE_UINT24 REMOTE_UINT16 ""sv};
constexpr static auto remoteSPICommand{"!sc" REMOTE_UINT8 REMOTE_UINT8 REMOTE_UINT16 REMOTE_UINT24 "#"sv};

bool fromHexSpan(const substrate::span<const char> &dataIn, substrate::span<uint8_t> dataOut) noexcept
{
	// If the ratio of data in to out is incorrect, fail early
	if (dataIn.size_bytes() < dataOut.size_bytes() * 2U)
		return false; // NOLINT(readability-simplify-boolean-expr)
	// Then iterate over the data to convert
	for (const auto offset : indexSequence_t{dataOut.size_bytes()})
	{
		// Convert a byte worth
		const toInt_t<uint8_t> value{dataIn.data() + (offset * 2U), 2U};
		if (!value.isHex())
			return false;
		// Then store the result
		dataOut[offset] = value.fromHex();
	}
	return true;
}

size_t toHexSpan(const substrate::span<const uint8_t> dataIn, substrate::span<char> &dataOut) noexcept
{
	// If the ratio of data in to out is incorrect, fail early
	if (dataIn.size_bytes() * 2U > dataOut.size_bytes())
		return 0U;
	// Then iterate over the data to convert
	for (const auto offset : indexSequence_t{dataIn.size_bytes()})
	{
		// Convert a byte worth
		const auto value{fromInt(dataIn[offset])};
		value.formatToHex(dataOut.subspan(offset * 2U, 2U), false);
	}
	// Return the number of chars in the dataOut span consumed by this
	return dataIn.size_bytes() * 2U;
}

template<typename T> bool fromHex(const substrate::span<const char> &dataIn, T &result) noexcept
	{ return fromHexSpan(dataIn, {reinterpret_cast<uint8_t *>(&result), sizeof(T)}); }
bool fromHex(const substrate::span<const char> &dataIn, void *const result, const size_t resultLength) noexcept
	{ return fromHexSpan(dataIn, {reinterpret_cast<uint8_t *>(result), resultLength}); }

size_t toHex(const void *const buffer, const size_t bufferLength, substrate::span<char> dataOut) noexcept
	{ return toHexSpan({reinterpret_cast<const uint8_t *>(buffer), bufferLength}, dataOut); }

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
	const toInt_t<uint64_t> version{versionString.data(), versionString.length() - 1U};
	if (!version.isHex())
		throw std::domain_error{"version value is not a hex number"s};
	return version.fromHex();
}

bool bmp_t::begin(const spiBus_t spiBus, const spiDevice_t spiDevice) noexcept
{
	const auto request{fmt::format(remoteSPIBegin, uint8_t(spiBus))};
	writePacket(request);
	const auto response{readPacket()};
	if (response[0] == remoteResponseOK)
	{
		_spiBus = spiBus;
		_spiDevice = spiDevice;
	}
	return response[0] == remoteResponseOK;
}

bool bmp_t::end() noexcept
{
	const auto request{fmt::format(remoteSPIEnd, uint8_t(_spiBus))};
	writePacket(request);
	const auto response{readPacket()};
	if (response[0] == remoteResponseOK)
	{
		_spiBus = spiBus_t::none;
		_spiDevice = spiDevice_t::none;
	}
	return response[0] == remoteResponseOK;
}

spiFlashID_t bmp_t::identifyFlash() const
{
	const auto request{fmt::format(remoteSPIChipID, uint8_t(_spiBus), uint8_t(_spiDevice))};
	writePacket(request);
	const auto response{readPacket()};
	if (response[0] != remoteResponseOK)
		throw bmpCommsError_t{};
	const auto chipID{std::string_view{response}.substr(1U)};
	if (chipID.length() != 7U)
		return {};
	spiFlashID_t result{};
	if (!fromHex(chipID, result))
		throw std::domain_error{"chip ID value is not a set of hex numbers"s};
	return result;
}

bool bmp_t::read(const spiFlashCommand_t command, const uint32_t address, void *const data,
	const size_t dataLength) const
{
	// XXX: Implement read chunking!
	if (dataLength > UINT16_MAX)
		return false;

	const auto request{fmt::format(remoteSPIRead, uint8_t(_spiBus), uint8_t(_spiDevice), uint16_t(command),
		address & 0x00ffffffU, dataLength)};
	writePacket(request);
	const auto response{readPacket()};
	// Check if the probe told us we asked for too big a read
	if (response[0] == remoteResponseParameterError)
		return false;
	// Check for any other errors
	if (response[0] != remoteResponseOK)
		throw bmpCommsError_t{};
	const auto resultData{std::string_view{response}.substr(1U)};
	if (!fromHex(resultData, data, dataLength))
		throw std::domain_error{"SPI read data is not properly hex encoded"s};
	return true;
}

bool bmp_t::write(const spiFlashCommand_t command, const uint32_t address, const void *const data,
	const size_t dataLength) const
{
	// XXX: Implement write chunking!
	if (dataLength > UINT16_MAX)
		return false;

	std::array<char, maxPacketSize + 1U> request{};
	auto offset
	{
		static_cast<size_t>
		(
			fmt::format_to_n(request.begin(), request.size(), remoteSPIWrite, uint8_t(_spiBus), uint8_t(_spiDevice),
				uint16_t(command), address & 0x00ffffffU, dataLength).out - request.begin()
		)
	};
	offset += toHex(data, dataLength, substrate::span{request}.subspan(offset, maxPacketSize - offset));
	request[offset] = '#';
	writePacket({request.data()});
	const auto response{readPacket()};
	// Check if the probe told us we asked for too big a read
	if (response[0] == remoteResponseParameterError)
		return false;
	// Check for any other errors
	if (response[0] != remoteResponseOK)
		throw bmpCommsError_t{};
	return true;
}

bool bmp_t::runCommand(const spiFlashCommand_t command, const uint32_t address) const
{
	const auto request{fmt::format(remoteSPICommand, uint8_t(_spiBus), uint8_t(_spiDevice), uint16_t(command),
		address & 0x00ffffffU)};
	writePacket(request);
	const auto response{readPacket()};
	// Check if the probe returned any kind of error response
	if (response[0] != remoteResponseOK)
		throw bmpCommsError_t{};
	return true;
}
