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

	[[nodiscard]] bool displayBasicParameterTable(const bmp_t &probe, const uint32_t address, const size_t length)
	{
		basicParameterTable_t parameterTable{};
		if (!sfdpRead(probe, address, &parameterTable, std::min(sizeof(basicParameterTable_t), length)))
			return false;

		console.info("Basic parameter table:");
		const auto [capacityValue, capacityUnits] = humanReadableSize(parameterTable.flashMemoryDensity.capacity());
		console.info("-> capacity "sv, capacityValue, capacityUnits);
		console.info("-> program page size: "sv, parameterTable.programmingAndChipEraseTiming.pageSize());
		console.info("-> sector erase opcode: "sv, asHex_t<2, '0'>(parameterTable.sectorEraseOpcode));
		console.info("-> supported erase types:"sv);
		for (const auto &[idx, eraseType] : indexedIterator_t{parameterTable.eraseTypes})
		{
			console.info("\t-> "sv, idx + 1U, ": "sv, nullptr);
			if (eraseType.eraseSizeExponent != 0U)
			{
				const auto [sizeValue, sizeUnits] = humanReadableSize(eraseType.eraseSize());
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
				if (!displayBasicParameterTable(probe, tableHeader.tableAddress, tableHeader.tableLength()))
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

	spiFlash_t readBasicParameterTable(const bmp_t &probe, const uint32_t address, const size_t length)
	{
		basicParameterTable_t parameterTable{};
		if (!sfdpRead(probe, address, &parameterTable, std::min(sizeof(basicParameterTable_t), length)))
			return {};

		const auto [sectorSize, sectorEraseOpcode]
		{
			[&]() -> std::tuple<size_t, uint8_t>
			{
				for (const auto &eraseType : parameterTable.eraseTypes)
				{
					if (eraseType.eraseSizeExponent != 0U && eraseType.opcode == parameterTable.sectorEraseOpcode)
						return {eraseType.eraseSize(), eraseType.opcode};
				}
				return {4_KiB, parameterTable.sectorEraseOpcode};
			}()
		};

		const auto pageSize{parameterTable.programmingAndChipEraseTiming.pageSize()};
		const auto capacity{parameterTable.flashMemoryDensity.capacity()};
		return {pageSize, sectorSize, sectorEraseOpcode, capacity};
	}

	std::optional<spiFlash_t> spiFlashRead(const bmp_t &probe)
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
				return readBasicParameterTable(probe, tableHeader.tableAddress, tableHeader.tableLength());
		}
		return std::nullopt;
	}
} // namespace bmpflash::sfdp
