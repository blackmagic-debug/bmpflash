// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 The Mangrove Language
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef ELF_TYPES_HXX
#define ELF_TYPES_HXX

#include <variant>
#include "io.hxx"
#include "enums.hxx"
#include "commonTypes.hxx"
#include "elf32Types.hxx"
#include "elf64Types.hxx"

/**
 * @file types.hxx
 * @brief High-level types representing the header structures and data in ELF files
 */

namespace bmpflash::elf::types
{
	using bmpflash::flags_t;

	struct elfHeader_t final
	{
	private:
		std::variant<elf32::elfHeader_t, elf64::elfHeader_t> _header;

	public:
		template<typename T> elfHeader_t(T header) noexcept : _header{header} { }

		[[nodiscard]] std::array<uint8_t, 4> magic() const noexcept;
		[[nodiscard]] class_t elfClass() const noexcept;
		[[nodiscard]] endian_t endian() const noexcept;
		[[nodiscard]] abi_t abi() const noexcept;
		[[nodiscard]] type_t type() const noexcept;
		[[nodiscard]] machine_t machine() const noexcept;
		[[nodiscard]] version_t version() const noexcept;
		[[nodiscard]] uint64_t entryPoint() const noexcept;
		[[nodiscard]] uint64_t phdrOffset() const noexcept;
		[[nodiscard]] uint64_t shdrOffset() const noexcept;
		[[nodiscard]] uint32_t flags() const noexcept;
		[[nodiscard]] uint16_t headerSize() const noexcept;
		[[nodiscard]] uint16_t programHeaderSize() const noexcept;
		[[nodiscard]] uint16_t programHeaderCount() const noexcept;
		[[nodiscard]] uint16_t sectionHeaderSize() const noexcept;
		[[nodiscard]] uint16_t sectionHeaderCount() const noexcept;
		[[nodiscard]] uint16_t sectionNamesIndex() const noexcept;
	};

	struct programHeader_t final
	{
	private:
		std::variant<elf32::programHeader_t, elf64::programHeader_t> _header;

	public:
		template<typename T> programHeader_t(T header) noexcept : _header{header} { }

		[[nodiscard]] programHeaderType_t type() const noexcept;
		[[nodiscard]] uint32_t flags() const noexcept;
		[[nodiscard]] uint64_t offset() const noexcept;
		[[nodiscard]] uint64_t virtualAddress() const noexcept;
		[[nodiscard]] uint64_t physicalAddress() const noexcept;
		[[nodiscard]] uint64_t fileLength() const noexcept;
		[[nodiscard]] uint64_t memoryLength() const noexcept;
		[[nodiscard]] uint64_t alignment() const noexcept;
	};

	struct sectionHeader_t final
	{
	private:
		std::variant<elf32::sectionHeader_t, elf64::sectionHeader_t> _header;

	public:
		template<typename T> sectionHeader_t(T header) noexcept : _header{header} { }

		[[nodiscard]] uint32_t nameOffset() const noexcept;
		[[nodiscard]] sectionHeaderType_t type() const noexcept;
		[[nodiscard]] flags_t<sectionFlag_t> flags() const noexcept;
		[[nodiscard]] uint64_t address() const noexcept;
		[[nodiscard]] uint64_t fileOffset() const noexcept;
		[[nodiscard]] uint64_t fileLength() const noexcept;
		[[nodiscard]] uint32_t link() const noexcept;
		[[nodiscard]] uint32_t info() const noexcept;
		[[nodiscard]] uint64_t alignment() const noexcept;
		[[nodiscard]] uint64_t entityLength() const noexcept;
	};

	struct elfSymbol_t final
	{
	private:
		std::variant<elf32::elfSymbol_t, elf64::elfSymbol_t> _header;

	public:
		template<typename T> elfSymbol_t(T header) noexcept : _header{header} { }

		[[nodiscard]] uint32_t nameOffset() const noexcept;
		[[nodiscard]] uint64_t value() const noexcept;
		[[nodiscard]] uint64_t symbolLength() const noexcept;
		[[nodiscard]] uint8_t info() const noexcept;
		[[nodiscard]] uint8_t other() const noexcept;
		[[nodiscard]] uint16_t sectionIndex() const noexcept;
	};

	struct stringTable_t final
	{
	private:
		memory_t _storage{{}};

	public:
		constexpr stringTable_t() noexcept = default;
		constexpr stringTable_t(const memory_t &storage) noexcept : _storage{storage} { }

		constexpr stringTable_t &operator =(const memory_t &storage) noexcept
		{
			_storage = storage;
			return *this;
		}

		[[nodiscard]] std::string_view stringFromOffset(size_t offset) const noexcept;
	};
} // namespace bmpflash::elf::types

#endif /*ELF_TYPES_HXX*/
