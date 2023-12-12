// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <string_view>
#include <substrate/console>
#include <substrate/index_sequence>
#include "bmp.hxx"
#include "spiFlash.hxx"

using namespace std::literals::string_view_literals;
using substrate::console;
using substrate::asHex_t;
using substrate::indexSequence_t;

namespace bmpflash::spiFlash
{
	bool spiFlash_t::waitFlashIdle(const bmp_t &probe)
	{
		uint8_t status = spiStatusBusy;
		while (status & spiStatusBusy)
		{
			if (!probe.read(spiFlashCommand_t::readStatus, 0U, &status, sizeof(status)))
			{
				console.error("Failed to read SPI Flash status"sv);
				return false;
			}
		}
		return true;
	}

	bool spiFlash_t::writeBlock(const bmp_t &probe, const size_t address, const substrate::span<uint8_t> &block)
	{
		console.debug("Erasing sector at 0x"sv, asHex_t<6, '0'>{address});
		// Start by erasing the block
		if (!probe.runCommand(spiFlashCommand_t::writeEnable, 0U) ||
			!probe.runCommand(spiFlashCommand_t::sectorErase | sectorEraseOpcode_, static_cast<uint32_t>(address)) ||
			!waitFlashIdle(probe))
		{
			console.error("Failed to prepare SPI Flash block for writing"sv);
			return false;
		}
		// Then loop through each write page worth of data in the block
		for (const auto offset : indexSequence_t{block.size()}.step(pageSize_))
		{
			// Try to enable write
			if (!probe.runCommand(spiFlashCommand_t::writeEnable, 0U))
			{
				console.error("Failed to prepare SPI Flash block for writing"sv);
				return false;
			}
			// Then run the page programming command with the block of data
			const auto subspan{block.subspan(offset, 256U)};
			console.debug("Writing "sv, subspan.size_bytes(), " bytes to page at 0x"sv,
				asHex_t<6, '0'>{address + offset});
			if (!probe.write(spiFlashCommand_t::pageProgram, static_cast<uint32_t>(address + offset),
				subspan.data(), subspan.size()) || !waitFlashIdle(probe))
			{
				console.error("Failed to write data to SPI Flash at offset +0x"sv, asHex_t{address + offset});
				return false;
			}
		}
		return true;
	}

	bool spiFlash_t::readBlock(const bmp_t &probe, const size_t address, substrate::span<uint8_t> block)
	{
		console.debug("Reading Flash starting at 0x"sv, asHex_t<6, '0'>{address});
		for (const auto offset : indexSequence_t{block.size()}.step(pageSize_))
		{
			auto subspan{block.subspan(offset, 256U)};
			if (!probe.read(spiFlashCommand_t::pageRead, static_cast<uint32_t>(address + offset),
				subspan.data(), subspan.size()))
			{
				console.error("Failed to read data from SPI Flash at offset +0x"sv, asHex_t{address + offset});
				return false;
			}
		}
		return true;
	}
} // namespace bmpflash::spiFlash
