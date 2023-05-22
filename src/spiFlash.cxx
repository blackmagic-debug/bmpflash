// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
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
	[[nodiscard]] bool waitFlashComplete(const bmp_t &probe)
	{
		uint8_t status = spiStatusBusy;
		while (status & spiStatusBusy)
		{
			if (!probe.read(spiFlashCommand_t::readStatus, 0U, &status, sizeof(status)))
			{
				console.error("Failed to read on-board Flash status"sv);
				return false;
			}
		}
		return true;
	}

	[[nodiscard]] bool writeBlock(const bmp_t &probe, const size_t address, const substrate::span<uint8_t> &block)
	{
		console.debug("Erasing sector at 0x"sv, asHex_t<6, '0'>{address});
		// Start by erasing the block
		if (!probe.runCommand(spiFlashCommand_t::writeEnable, 0U) ||
			!probe.runCommand(spiFlashCommand_t::sectorErase, static_cast<uint32_t>(address)) ||
			!waitFlashComplete(probe))
		{
			console.error("Failed to prepare on-board Flash block for writing"sv);
			return false;
		}
		// Then loop through each write page worth of data in the block
		for (const auto offset : indexSequence_t{block.size()}.step(256))
		{
			// Try to enable write
			if (!probe.runCommand(spiFlashCommand_t::writeEnable, 0U))
			{
				console.error("Failed to prepare on-board Flash block for writing"sv);
				return false;
			}
			// Then run the page programming command with the block of data
			const auto subspan{block.subspan(offset, 256U)};
			console.debug("Writing "sv, subspan.size_bytes(), " bytes to page at 0x"sv,
				asHex_t<6, '0'>{address + offset});
			if (!probe.write(spiFlashCommand_t::pageProgram, static_cast<uint32_t>(address + offset),
				subspan.data(), subspan.size()) || !waitFlashComplete(probe))
			{
				console.error("Failed to write data to on-board Flash at offset +0x"sv, asHex_t{address + offset});
				return false;
			}
		}
		return true;
	}
} // namespace bmpflash::spiFlash
