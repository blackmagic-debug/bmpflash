// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <cstddef>
#include <string_view>
#include <map>
#include <optional>
#include <substrate/fd>
#include <substrate/console>
#include <substrate/index_sequence>
#include <substrate/indexed_iterator>
#include "provisionELF.hxx"

using namespace std::literals::string_view_literals;
using substrate::asHex_t;
using substrate::indexSequence_t;
using substrate::indexedIterator_t;

namespace bmpflash::elf
{
	using segmentMap_t = std::map<uint64_t, const programHeader_t &>;

	provision_t::provision_t(const path &fileName) noexcept : file{fd_t{fileName.c_str(), O_RDONLY | O_NOCTTY}} { }

	bool provision_t::valid() const noexcept
	{
		const auto &elfHeader{file.header()};
		if (elfHeader.magic() != elfMagic)
		{
			console.error("File is not a valid ELF file"sv);
			return false;
		}

		// XXX: This only allows 32-bit little endian ELF files, while we actually need to be able to
		//      consume a variety of 32- and 64-bit files in either endian (which we allow needs to
		//      depend on a table of targets).
		return
			elfHeader.elfClass() == class_t::elf32Bit && elfHeader.endian() == endian_t::little &&
			elfHeader.version() == version_t::current && elfHeader.abi() == abi_t::systemV &&
			elfHeader.abiVersion() == 0U;
	}

	std::optional<segmentMap_t> collectSegments(const elf_t &file) noexcept
	{
		segmentMap_t segmentMap{};
		for (const auto &[headerIndex, progHeader] : indexedIterator_t{file.programHeaders()})
		{
			if (progHeader.fileLength() >= UINT32_C(0xffffffff) ||
				progHeader.offset() >= UINT32_C(0xffffffff))
			{
				console.error("Reading program header for chunk "sv, headerIndex, " failed"sv);
				return std::nullopt;
			}

			if (progHeader.type() == programHeaderType_t::load && progHeader.fileLength() != 0)
				segmentMap.emplace(progHeader.virtualAddress(), progHeader);
		}
		return segmentMap;
	}

	[[nodiscard]] auto map(const segmentMap_t &segmentMap, const sectionHeader_t &sectHeader)
	{
		for (auto begin{segmentMap.begin()}; begin != segmentMap.end(); ++begin)
		{
			const auto &[_, progHeader] = *begin;
			const auto progHdrEnd{progHeader.virtualAddress() + progHeader.memoryLength()};
			const auto sectHdrEnd{sectHeader.address() + sectHeader.fileLength()};
			if (sectHeader.address() >= progHeader.virtualAddress() && sectHdrEnd <= progHdrEnd)
				return begin;
		}
		return segmentMap.end();
	}

	[[nodiscard]] bool writeBlock(const bmp_t &probe, const size_t address, const span<uint8_t> &block)
	{
		// Start by erasing the block
		if (!probe.runCommand(spiFlashCommand_t::writeEnable, 0U) ||
			!probe.runCommand(spiFlashCommand_t::sectorErase, address))
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
			if (!probe.write(spiFlashCommand_t::pageProgram, address + offset, subspan.data(), subspan.size()))
			{
				console.error("Failed to write data to on-board Flash at offset +0x"sv, asHex_t{address + offset});
				return false;
			}
		}
		return true;
	}
	bool provision_t::repack(const bmp_t &probe) const noexcept
	{
		const auto elfHeader{file.header()};
		if (elfHeader.type() != type_t::executable || elfHeader.machine() != machine_t::arm ||
			elfHeader.version() != version_t::current)
		{
			console.error("File does not contain a valid firmware image"sv);
			return false;
		}

		const auto segmentMap{collectSegments(file)};
		if (!segmentMap)
			return false;
		console.info("Found "sv, segmentMap->size(), " usable program headers"sv);
		return true;
	}
} // namespace bmpflash::elf
