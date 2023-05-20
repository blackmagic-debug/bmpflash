// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 The Mangrove Language
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include "types.hxx"

using namespace bmpflash::elf::types;
using bmpflash::elf::io::match_t;

// NOLINTBEGIN(bugprone-exception-escape)

std::array<uint8_t, 4> elfHeader_t::magic() const noexcept
	{ return std::visit([](const auto &header) { return header.magic(); }, _header); }
class_t elfHeader_t::elfClass() const noexcept
	{ return std::visit([](const auto &header) { return header.elfClass(); }, _header); }
endian_t elfHeader_t::endian() const noexcept
	{ return std::visit([](const auto &header) { return header.endian(); }, _header); }
abi_t elfHeader_t::abi() const noexcept
	{ return std::visit([](const auto &header) { return header.abi(); }, _header); }
uint8_t elfHeader_t::abiVersion() const noexcept
	{ return std::visit([](const auto &header) { return header.abiVersion(); }, _header); }
type_t elfHeader_t::type() const noexcept
	{ return std::visit([](const auto &header) { return header.type(); }, _header); }
machine_t elfHeader_t::machine() const noexcept
	{ return std::visit([](const auto &header) { return header.machine(); }, _header); }
version_t elfHeader_t::version() const noexcept
	{ return std::visit([](const auto &header) { return header.version(); }, _header); }
uint64_t elfHeader_t::entryPoint() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.entryPoint(); }, _header); }
uint64_t elfHeader_t::phdrOffset() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.phdrOffset(); }, _header); }
uint64_t elfHeader_t::shdrOffset() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.shdrOffset(); }, _header); }
uint32_t elfHeader_t::flags() const noexcept
	{ return std::visit([](const auto &header) { return header.flags(); }, _header); }
uint16_t elfHeader_t::headerSize() const noexcept
	{ return std::visit([](const auto &header) { return header.headerSize(); }, _header); }
uint16_t elfHeader_t::programHeaderSize() const noexcept
	{ return std::visit([](const auto &header) { return header.programHeaderSize(); }, _header); }
uint16_t elfHeader_t::programHeaderCount() const noexcept
	{ return std::visit([](const auto &header) { return header.programHeaderCount(); }, _header); }
uint16_t elfHeader_t::sectionHeaderSize() const noexcept
	{ return std::visit([](const auto &header) { return header.sectionHeaderSize(); }, _header); }
uint16_t elfHeader_t::sectionHeaderCount() const noexcept
	{ return std::visit([](const auto &header) { return header.sectionHeaderCount(); }, _header); }
uint16_t elfHeader_t::sectionNamesIndex() const noexcept
	{ return std::visit([](const auto &header) { return header.sectionNamesIndex(); }, _header); }

programHeaderType_t programHeader_t::type() const noexcept
	{ return std::visit([](const auto &header) { return header.type(); }, _header); }
uint32_t programHeader_t::flags() const noexcept
	{ return std::visit([](const auto &header) { return header.flags(); }, _header); }
uint64_t programHeader_t::offset() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.offset(); }, _header); }
uint64_t programHeader_t::virtualAddress() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.virtualAddress(); }, _header); }
uint64_t programHeader_t::physicalAddress() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.physicalAddress(); }, _header); }
uint64_t programHeader_t::fileLength() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.fileLength(); }, _header); }
uint64_t programHeader_t::memoryLength() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.memoryLength(); }, _header); }
uint64_t programHeader_t::alignment() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.alignment(); }, _header); }

uint32_t sectionHeader_t::nameOffset() const noexcept
	{ return std::visit([](const auto &header) { return header.nameOffset(); }, _header); }
sectionHeaderType_t sectionHeader_t::type() const noexcept
	{ return std::visit([](const auto &header) { return header.type(); }, _header); }
flags_t<sectionFlag_t> sectionHeader_t::flags() const noexcept
	{ return std::visit([](const auto &header) { return header.flags(); }, _header); }
uint64_t sectionHeader_t::address() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.address(); }, _header); }
uint64_t sectionHeader_t::fileOffset() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.fileOffset(); }, _header); }
uint64_t sectionHeader_t::fileLength() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.fileLength(); }, _header); }
uint32_t sectionHeader_t::link() const noexcept
	{ return std::visit([](const auto &header) { return header.link(); }, _header); }
uint32_t sectionHeader_t::info() const noexcept
	{ return std::visit([](const auto &header) { return header.info(); }, _header); }
uint64_t sectionHeader_t::alignment() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.alignment(); }, _header); }
uint64_t sectionHeader_t::entityLength() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.entityLength(); }, _header); }

uint32_t elfSymbol_t::nameOffset() const noexcept
	{ return std::visit([](const auto &header) { return header.nameOffset(); }, _header); }
uint64_t elfSymbol_t::value() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.value(); }, _header); }
uint64_t elfSymbol_t::symbolLength() const noexcept
	{ return std::visit([](const auto &header) -> uint64_t { return header.symbolLength(); }, _header); }
uint8_t elfSymbol_t::info() const noexcept
	{ return std::visit([](const auto &header) { return header.info(); }, _header); }
uint8_t elfSymbol_t::other() const noexcept
	{ return std::visit([](const auto &header) { return header.other(); }, _header); }
uint16_t elfSymbol_t::sectionIndex() const noexcept
	{ return std::visit([](const auto &header) { return header.sectionIndex(); }, _header); }

std::string_view stringTable_t::stringFromOffset(const size_t offset) const noexcept
{
	// Start by getting a subspan at the correct offset
	const auto data{_storage.dataSpan().subspan(offset)};
	// Now, we need to figure out how long the string is.. we could use strlen(),
	// but that would introduce a risk of memory unsafety here, so we instead iterate
	// till we find '\0' or fall off the end of the span, which should mean we
	// can't accidentally do a memory safety bad here.
	size_t length{};
	for (const auto &value : data)
	{
		++length;
		if (value == 0x00U)
			break;
	}
	// Finally, put the whole thing together as a string view
	// NOTLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
	return {reinterpret_cast<const char *>(data.data()), length};
}

// NOLINTEND(bugprone-exception-escape)
