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
#include <substrate/units>
#include <substrate/buffer_utils>
#include "provisionELF.hxx"
#include "crc32.hxx"
#include "sfdp.hxx"

using namespace std::literals::string_view_literals;
using substrate::asHex_t;
using substrate::indexSequence_t;
using substrate::indexedIterator_t;
using substrate::operator ""_KiB;
using substrate::buffer_utils::writeLE;
using bmpflash::spiFlash::spiFlash_t;

namespace bmpflash::elf
{
	constexpr static std::array<char, 4> flashMagic{{'B', 'M', 'P', 'F'}};

	struct flashSection_t final
	{
		uint32_t offset{};
		uint32_t length{};
		uint64_t flashAddr{};
	};

	/*
	 * The Flash header is laid out as follows:
	 *
	 *   0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	 * +---+---+---+---+---------------+---------------+----------------+
	 * | B | M | P | F |     CRC32     | section count | ....           | +0x0
	 * +---+---+---+---+---------------+---------------+----------------+
	 *
	 * Where '....' at the end signals the start of the packed flashSection_t headers.
	 *
	 * The CRC32 value covers all bytes in the page and is calculated assuming the
	 * CRC32 is value 0xfffffffFU.
	 */
	struct flashHeader_t final
	{
		// NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
		std::vector<flashSection_t> sections{};
		[[nodiscard]] bool toPage(span<uint8_t> pageBuffer) const noexcept;
	};

	using segmentMap_t = std::map<uint64_t, const programHeader_t &>;
	using block_t = std::array<uint8_t, 4_KiB>;

	provision_t::provision_t(const path &fileName) noexcept : file{fd_t{fileName, O_RDONLY | O_NOCTTY}} { }

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

	[[nodiscard]] uint32_t currentOffsetFrom(const flashHeader_t &flashHeader) noexcept
	{
		// If there are no sections stored yet, the offset of the first is at the start of the second erase block
		if (flashHeader.sections.empty())
			return 4_KiB;
		// Otherwise, grab the last and compute the next offset
		const auto &last{flashHeader.sections.back()};
		const auto offset{last.offset + last.length};
		// Adjust the resulting value to the start of the next erase block
		return static_cast<uint32_t>((offset + (4_KiB - 1U)) & ~(4_KiB - 1U));
	}

	bool packSection(const elf_t &file, const bmp_t &probe, spiFlash_t &spiFlash, flashHeader_t &flashHeader,
		const size_t sectionIndex, const segmentMap_t &segmentMap)
	{
		const auto &sectHeader{file.sectionHeaders()[sectionIndex]};
		const auto sectName{file.sectionNames().stringFromOffset(sectHeader.nameOffset())};
		console.debug("Looking for section "sv, sectionIndex, " ("sv, sectName,
			") in segment map. Section has address 0x"sv, asHex_t<8, '0'>{sectHeader.address()});
		auto segment{map(segmentMap, sectHeader)};
		if (segment == segmentMap.end() || !sectHeader.fileOffset())
			return true;
		const auto &progHeader{segment->second};

		if (sectHeader.fileLength() == 0)
		{
			console.debug("Section is empty, skipping"sv);
			return true;
		}

		flashSection_t flashSection{};
		flashSection.offset = currentOffsetFrom(flashHeader);
		flashSection.flashAddr = progHeader.physicalAddress() + (sectHeader.address() - progHeader.virtualAddress());

		console.debug("Found section in segment map, attempting to get underlying data for it"sv);
		const auto sectionData{file.dataFor(sectHeader)};
		if (sectionData.empty())
		{
			console.error("Cannot get any underlying data for section "sv, sectionIndex, " ("sv, sectName,
				") at address "sv, asHex_t{sectHeader.address()});
			return false;
		}

		console.debug("Transfering "sv, sectionData.size(), " bytes of data to on-board Flash at offset +0x"sv,
			asHex_t{flashSection.offset});
		block_t segmentBuffer{};
		for (const auto offset : indexSequence_t{sectionData.size()}.step(segmentBuffer.size()))
		{
			// Figure out how many bytes we can sling at the segment buffer
			const size_t amount{std::min(segmentBuffer.size(), sectionData.size() - offset)};
			// Request the appropriate subspan from the sectionData span
			const auto subspan{sectionData.subspan(offset, amount)};
			// Copy the data, and then fill any remaining space with the Flash blank constant
			std::copy(subspan.begin(), subspan.end(), segmentBuffer.begin());
			for (const auto &idx : substrate::indexSequence_t{subspan.size(), segmentBuffer.size()})
				segmentBuffer[idx] = 0xffU;

			const auto blockOffset{flashSection.offset + offset};
			if (!spiFlash.writeBlock(probe, blockOffset, segmentBuffer))
			{
				console.error("Failed to write segment data for 0x"sv, asHex_t{sectHeader.address()}, "+0x"sv,
					asHex_t{offset}, " to the on-board Flash at offset +"sv, asHex_t{blockOffset});
				return false;
			}
		}
		flashSection.length = static_cast<uint32_t>(sectionData.size_bytes());
		console.info("Adding section at "sv, asHex_t{flashSection.offset}, '(',
			asHex_t{flashSection.length}, ") to flash header"sv);
		flashHeader.sections.push_back(flashSection);
		return true;
	}

	bool provision_t::repack(const bmp_t &probe) const
	{
		const auto elfHeader{file.header()};
		if (elfHeader.type() != type_t::executable || elfHeader.machine() != machine_t::arm ||
			elfHeader.version() != version_t::current)
		{
			console.error("File does not contain a valid firmware image"sv);
			return false;
		}

		auto spiFlash{sfdp::read(probe)};
		if (!spiFlash)
		{
			console.error("Could not setup SPI Flash control structures"sv);
			return false;
		}

		const auto segmentMap{collectSegments(file)};
		if (!segmentMap)
			return false;
		console.info("Found "sv, segmentMap->size(), " usable program headers"sv);

		flashHeader_t flashHeader{};
		const auto sectHeaderCount{file.sectionHeaders().size()};
		console.info("Found "sv, sectHeaderCount, " section headers"sv);

		for (const auto &headerIndex : substrate::indexSequence_t{sectHeaderCount})
		{
			if (!packSection(file, probe, *spiFlash, flashHeader, headerIndex, *segmentMap))
				return false;
		}

		block_t headerBuffer{};
		if (!flashHeader.toPage(headerBuffer) ||
			!spiFlash->writeBlock(probe, 0U, headerBuffer))
		{
			console.error("Failed to write the Flash header to the on-board Flash"sv);
			return false;
		}
		return true;
	}

	template<typename T> void copyInto(span<uint8_t> destination, const span<const T> &source) noexcept
	{
		std::memcpy(destination.data(), source.data(),
			std::min(destination.size_bytes(), source.size_bytes()));
	}

	template<typename T, size_t N> void copyInto(span<uint8_t> destination, const std::array<T, N> &source) noexcept
		{ return copyInto(destination, span<const T>{source.data(), source.size()}); }

	bool flashHeader_t::toPage(span<uint8_t> pageBuffer) const noexcept try
	{
		std::fill(pageBuffer.begin(), pageBuffer.end(), 0xffU);
		copyInto(pageBuffer.subspan(0, 4), flashMagic);
		writeLE(static_cast<uint32_t>(sections.size()), pageBuffer.subspan(8, 4));

		size_t offset{12U};
		for (const auto &section : sections)
		{
			static_assert(sizeof(section) == 16U);
			const auto sectionSubspan{pageBuffer.subspan(offset, sizeof(section))};
			if (sectionSubspan.size_bytes() > pageBuffer.size_bytes())
				return false;
			writeLE(section.offset, sectionSubspan.subspan(0, 4));
			writeLE(section.length, sectionSubspan.subspan(4, 4));
			writeLE(section.flashAddr, sectionSubspan.subspan(8, 8));
			offset += sizeof(section);
		}

		uint32_t crc32{0xffffffffU};
		crc32_t::crc(crc32, {pageBuffer.data(), pageBuffer.size()});
		writeLE(crc32, pageBuffer.subspan(4, 4));
		return true;
	}
	catch (const std::out_of_range &)
		{ return false; }
} // namespace bmpflash::elf
