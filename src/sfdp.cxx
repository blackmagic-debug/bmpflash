// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <substrate/console>
#include <substrate/index_sequence>
#include <substrate/indexed_iterator>
#include <substrate/units>
#include "sfdp.hxx"
#include "sfdpInternal.hxx"
#include "units.hxx"

using namespace std::literals::string_view_literals;
using substrate::console;
using substrate::asHex_t;
using substrate::indexSequence_t;
using substrate::indexedIterator_t;
using substrate::operator ""_KiB;
using bmpflash::utils::humanReadableSize;

namespace bmpflash::sfdp
{
	constexpr static uint32_t sfdpHeaderAddress{0U};
	constexpr static uint32_t tableHeaderAddress{sizeof(sfdpHeader_t)};

	constexpr static std::array<char, 4> sfdpMagic{{'S', 'F', 'D', 'P'}};
	constexpr static uint16_t basicSPIParameterTable{0xFF00U};

	[[nodiscard]] bool sfdpRead(const bmp_t &probe, const uint32_t address, void *const data,
			const size_t dataLength)
		{ return probe.read(spiFlashCommand_t::readSFDP, address, data, dataLength); }

	template<typename T> [[nodiscard]] bool sfdpRead(const bmp_t &probe, const uintptr_t address, T &buffer)
			{ return sfdpRead(probe, static_cast<uint32_t>(address), &buffer, sizeof(T)); }

	void displayHeader(const sfdpHeader_t &header)
	{
		console.info("SFDP Header:"sv);
		console.info("-> magic '"sv, header.magic, "'"sv);
		console.info("-> version "sv, header.versionMajor, '.', header.versionMinor);
		console.info("-> "sv, header.parameterHeadersCount(), " parameter headers"sv);
		console.info("-> access protocol "sv, asHex_t<2, '0'>{uint8_t(header.accessProtocol)});
	}

	void displayTableHeader(const parameterTableHeader_t &header, const size_t index)
	{
		console.info("Parameter table header "sv, index, ":"sv);
		console.info("-> type "sv, asHex_t<4, '0'>{header.jedecParameterID()});
		console.info("-> version "sv, header.versionMajor, '.', header.versionMinor);
		console.info("-> table is "sv, header.tableLength(), " bytes long"sv);
		console.info("-> table SFDP address: "sv, uint32_t{header.tableAddress});
	}

	[[nodiscard]] bool displayBasicParameterTable(const bmp_t &probe, const parameterTableHeader_t &header)
	{
		basicParameterTable_t parameterTable{};
		if (!sfdpRead(probe, header.tableAddress, &parameterTable,
			std::min(sizeof(basicParameterTable_t), header.tableLength())))
			return false;

		console.info("Basic parameter table:");
		const auto [capacityValue, capacityUnits] =
			humanReadableSize(static_cast<size_t>(parameterTable.flashMemoryDensity.capacity()));
		console.info("-> capacity "sv, capacityValue, capacityUnits);
		if (header.versionMajor > 1U || (header.versionMajor == 1U && header.versionMinor >= 5U))
			console.info("-> program page size: "sv, parameterTable.programmingAndChipEraseTiming.pageSize());
		else
			console.info("-> program page size: default (256)"sv);
		console.info("-> sector erase opcode: "sv, asHex_t<2, '0'>(parameterTable.sectorEraseOpcode));
		console.info("-> supported erase types:"sv);
		for (const auto &[idx, eraseType] : indexedIterator_t{parameterTable.eraseTypes})
		{
			console.info("\t-> "sv, idx + 1U, ": "sv, nullptr);
			if (eraseType.eraseSizeExponent != 0U)
			{
				const auto [sizeValue, sizeUnits] = humanReadableSize(static_cast<size_t>(eraseType.eraseSize()));
				console.writeln("opcode "sv, asHex_t<2, '0'>(eraseType.opcode), ", erase size: "sv,
					sizeValue, sizeUnits);
			}
			else
				console.writeln("invalid erase type"sv);
		}
		console.info("-> power down opcode: "sv, asHex_t<2, '0'>(parameterTable.deepPowerdown.enterInstruction()));
		console.info("-> wake up opcode: "sv, asHex_t<2, '0'>(parameterTable.deepPowerdown.exitInstruction()));
		return true;
	}

	bool readAndDisplay(const bmp_t &probe)
	{
		console.info("Reading SFDP data for device"sv);
		sfdpHeader_t header{};
		if (!sfdpRead(probe, sfdpHeaderAddress, header))
			return false;
		if (header.magic != sfdpMagic)
		{
			console.error("Device does not have a valid SFDP block"sv);
			console.error(" -> Read signature '"sv, header.magic, "'");
			console.error(" -> Expected signature '"sv, sfdpMagic, "'");
			return true;
		}
		displayHeader(header);

		for (const auto idx : indexSequence_t{header.parameterHeadersCount()})
		{
			parameterTableHeader_t tableHeader{};
			if (!sfdpRead(probe, tableHeaderAddress + (sizeof(parameterTableHeader_t) * idx), tableHeader))
				return false;
			displayTableHeader(tableHeader, idx + 1U);
			if (tableHeader.jedecParameterID() == basicSPIParameterTable)
			{
				tableHeader.validate();
				if (!displayBasicParameterTable(probe, tableHeader))
					return false;
			}
		}
		return true;
	}

	std::optional<spiFlash_t> spiFlashFromID(const bmp_t &probe)
	{
		const auto chipID{probe.identifyFlash()};
		// If we got a bad all-highs read back, or the capacity is 0, then there's no device there.
		if ((chipID.manufacturer == 0xffU && chipID.type == 0xffU && chipID.capacity == 0xffU) ||
			chipID.capacity == 0U)
		{
			console.error("Failed to read JEDEC ID"sv);
			return std::nullopt;
		}
		const auto flashSize{UINT32_C(1) << chipID.capacity};
		return {flashSize};
	}

	spiFlash_t readBasicParameterTable(const bmp_t &probe, const parameterTableHeader_t &header)
	{
		basicParameterTable_t parameterTable{};
		if (!sfdpRead(probe, header.tableAddress, &parameterTable,
			std::min(sizeof(basicParameterTable_t), header.tableLength())))
			return {};

		const auto [sectorSize, sectorEraseOpcode]
		{
			[&]() -> std::tuple<uint64_t, uint8_t>
			{
				for (const auto &eraseType : parameterTable.eraseTypes)
				{
					if (eraseType.eraseSizeExponent != 0U && eraseType.opcode == parameterTable.sectorEraseOpcode)
						return {eraseType.eraseSize(), eraseType.opcode};
				}
				return {4_KiB, parameterTable.sectorEraseOpcode};
			}()
		};

		const auto pageSize
		{
			[&]() noexcept -> uint64_t
			{
				if (header.versionMajor > 1U || (header.versionMajor == 1U && header.versionMinor >= 5U))
					return parameterTable.programmingAndChipEraseTiming.pageSize();
				return 256U;
			}()
		};
		const auto capacity{parameterTable.flashMemoryDensity.capacity()};
		return {pageSize, sectorSize, sectorEraseOpcode, capacity};
	}

	std::optional<spiFlash_t> read(const bmp_t &probe)
	{
		console.info("Reading SFDP data for device"sv);
		sfdpHeader_t header{};
		if (!sfdpRead(probe, sfdpHeaderAddress, header))
			return std::nullopt;
		if (header.magic != sfdpMagic)
		{
			console.warn("Failed to read SFDP data, falling back on JEDEC ID"sv);
			return spiFlashFromID(probe);
		}

		for (const auto idx : indexSequence_t{header.parameterHeadersCount()})
		{
			parameterTableHeader_t tableHeader{};
			if (!sfdpRead(probe, tableHeaderAddress + (sizeof(parameterTableHeader_t) * idx), tableHeader))
				return std::nullopt;

			if (tableHeader.jedecParameterID() == basicSPIParameterTable)
			{
				tableHeader.validate();
				return readBasicParameterTable(probe, tableHeader);
			}
		}
		return std::nullopt;
	}

	size_t parameterTableHeader_t::lengthForVersion() const noexcept
	{
		if (versionMajor < 1U)
		{
			console.warn("SFDP basic parameters table header version incorrect, got v"sv, versionMajor, '.',
				versionMinor, " which is less than minimum allowable version of v1.0"sv);
			// If the version number is impossible, just return the table length - there's nothing else we can do.
			return tableLength();
		}
		// Turn the version number into a uint16_t with the upper byte being the major and the lower being the minor
		const auto version{static_cast<uint16_t>((versionMajor << 8U) | versionMinor)};
		// Now switch on the valid ones we know about
		switch (version)
		{
			// v1.0 through v1.4 from the original JESD216
			case 0x0100U:
			case 0x0101U:
			case 0x0102U:
			case 0x0103U:
			case 0x0104U:
				return 36U; // 9 uint32_t's
			// v1.5 (JESD216A), v1.6 (JESD216B)
			case 0x0105U:
			case 0x0106U:
				return 64U; // 16 uint32_t's
			// v1.7 (JESD216C, JESD216D, JESD216E)
			case 0x0107U:
				return 84U; // 21 uint32_t's
			// v1.8 (JESD216F)
			case 0x0108U:
				return 96U; // 24 uint32_t's
			default:
				console.warn("Unknown SFDP version v"sv, versionMajor, '.', versionMinor, ", assuming valid size"sv);
				return tableLength();
		}
	}

	void parameterTableHeader_t::validate() noexcept
	{
		const auto expectedLength{lengthForVersion()};
		const auto actualLength{tableLength()};
		// If the table is the proper length for the version, we're done
		if (actualLength == expectedLength)
			return;

		// If the table is longer than it should be for the stated version, truncate it
		if (actualLength > expectedLength)
			tableLengthInU32s = static_cast<uint8_t>(expectedLength / 4U);
		// Otherwise fix the version number to match the one for the actual length
		else
		{
			// 24 uint32_t's -> v1.8
			if (actualLength == 96U)
			{
				versionMajor = 1U;
				versionMinor = 8U;
			}
			// 21 uint32_t's -> v1.7
			else if (actualLength == 84U)
			{
				versionMajor = 1U;
				versionMinor = 7U;
			}
			// 16 uint32_t's -> v1.6 (assume the newer standard)
			else if (actualLength == 64U)
			{
				versionMajor = 1U;
				versionMinor = 6U;
			}
			// 9 uint32_t's -> v1.4 (assume the newer standard)
			else if (actualLength == 36U)
			{
				versionMajor = 1U;
				versionMinor = 4U;
			}
			else
				console.error("This should not be possible, please check sfdp.cxx for sanity"sv);
			console.info("Adjusted version is "sv, versionMajor, '.', versionMinor);
		}
	}
} // namespace bmpflash::sfdp
