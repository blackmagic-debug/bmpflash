// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 The Mangrove Language
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <substrate/index_sequence>
#include "elf.hxx"

namespace bmpflash::elf
{
	using bmpflash::elf::io::match_t;

	elf_t::elf_t(fd_t &&file) : _backingStorage{file.map(PROT_READ)}, _header
	{
		[this]() -> elfHeader_t
		{
			auto &map{std::get<mmap_t>(_backingStorage)};
			const auto data{toSpan(map)};
			const elfIdent_t ident{data};
			// TODO: Check the validity of ident
			if (ident.elfClass() == class_t::elf32Bit)
				return elf32::elfHeader_t{data};
			return elf64::elfHeader_t{data};
		}()
	} // This initially allows the vectors for the headers to be default constructed.
	{
		// Validate the header read in the previous step, including the sizes used for the program and section headers.
		//if (!_header.valid())
		//	throw ;

		const auto data{toSpan(std::get<mmap_t>(_backingStorage))};
		const auto elfClass{_header.elfClass()};
		const auto endian{_header.endian()};
		const auto programHeaderSize{_header.programHeaderSize()};

		size_t offset{_header.phdrOffset()};
		// Once the elfHeader_t has been read and handled, we then loop through pulling out the program headers.
		for ([[maybe_unused]] const auto index : substrate::indexSequence_t{_header.programHeaderCount()})
		{
			if (elfClass == class_t::elf32Bit)
				_programHeaders.emplace_back
				(
					elf32::programHeader_t
					{
						data.subspan(offset, elf32::programHeader_t::size()),
						endian
					}
				);
			else
				_programHeaders.emplace_back
				(
					elf64::programHeader_t
					{
						data.subspan(offset, elf64::programHeader_t::size()),
						endian
					}
				);
			offset += programHeaderSize;
		}

		const auto sectionHeaderSize{_header.sectionHeaderSize()};
		offset = _header.shdrOffset();
		// Now loop through and pull out all the section headers.
		for ([[maybe_unused]] const auto index : substrate::indexSequence_t{_header.sectionHeaderCount()})
		{
			if (elfClass == class_t::elf32Bit)
				_sectionHeaders.emplace_back
				(
					elf32::sectionHeader_t
					{
						data.subspan(offset, elf32::sectionHeader_t::size()),
						endian
					}
				);
			else
				_sectionHeaders.emplace_back
				(
					elf64::sectionHeader_t
					{
						data.subspan(offset, elf64::sectionHeader_t::size()),
						endian
					}
				);
			offset += sectionHeaderSize;
		}

		// Extract the section names
		const auto &sectionNamesHeader{_sectionHeaders[_header.sectionNamesIndex()]};
		_sectionNames = data.subspan(sectionNamesHeader.fileOffset(), sectionNamesHeader.fileLength());
	}

	elf_t::elf_t(const class_t elfClass) : _backingStorage{fragmentStorage_t{}}, _header
	{
		[this](const class_t fileClass) -> elfHeader_t
		{
			if (fileClass == class_t::elf32Bit)
				return allocate<elf32::elfHeader_t>();
			return allocate<elf64::elfHeader_t>();
		}(elfClass)
	} { }

	[[nodiscard]] span<uint8_t> elf_t::dataFor(const programHeader_t &header) noexcept try
	{
		return std::visit(match_t
		{
			[&](mmap_t &storage)
			{
				const auto &data{toSpan(storage)};
				return data.subspan(header.offset(), header.fileLength());
			},
			[&](const fragmentStorage_t &) -> span<uint8_t> { return {}; }
		}, _backingStorage);
	}
	catch (const std::out_of_range &)
		{ return {}; }

	[[nodiscard]] span<const uint8_t> elf_t::dataFor(const programHeader_t &header) const noexcept try
	{
		return std::visit(match_t
		{
			[&](const mmap_t &storage)
			{
				const auto &data{toSpan(storage)};
				return data.subspan(header.offset(), header.fileLength());
			},
			[&](const fragmentStorage_t &) -> span<const uint8_t> { return {}; }
		}, _backingStorage);
	}
	catch (const std::out_of_range &)
		{ return {}; }

	[[nodiscard]] span<uint8_t> elf_t::dataFor(const sectionHeader_t &header) noexcept try
	{
		return std::visit(match_t
		{
			[&](mmap_t &storage)
			{
				const auto &data{toSpan(storage)};
				return data.subspan(header.fileOffset(), header.fileLength());
			},
			[&](const fragmentStorage_t &) -> span<uint8_t> { return {}; }
		}, _backingStorage);
	}
	catch (const std::out_of_range &)
		{ return {}; }

	[[nodiscard]] span<const uint8_t> elf_t::dataFor(const sectionHeader_t &header) const noexcept try
	{
		return std::visit(match_t
		{
			[&](const mmap_t &storage)
			{
				const auto &data{toSpan(storage)};
				return data.subspan(header.fileOffset(), header.fileLength());
			},
			[&](const fragmentStorage_t &) -> span<const uint8_t> { return {}; }
		}, _backingStorage);
	}
	catch (const std::out_of_range &)
		{ return {}; }
} // namespace bmpflash::elf
