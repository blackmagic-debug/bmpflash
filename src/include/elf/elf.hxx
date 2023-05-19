// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 The Mangrove Language
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef FORMATS_ELF_HXX
#define FORMATS_ELF_HXX

#include <variant>
#include <vector>
#include <memory>
#include <substrate/fd>
#include <substrate/mmap>
#include <substrate/span>
#include "types.hxx"

namespace bmpflash::elf
{
	inline namespace internal
	{
		using substrate::fd_t;
		using substrate::span;
		using namespace types;

		using substrate::mmap_t;
		// NOLINTNEXTLINE(modernize-avoid-c-arrays)
		using fragmentStorage_t = std::vector<std::unique_ptr<uint8_t []>>;

		[[nodiscard]] static inline span<uint8_t> toSpan(mmap_t &map) noexcept
			{ return {map.address<uint8_t>(), map.length()}; }
	} // namespace internal

	struct elf_t final
	{
	private:
		std::variant<mmap_t, fragmentStorage_t> _backingStorage;
		elfHeader_t _header;
		std::vector<programHeader_t> _programHeaders{};
		std::vector<sectionHeader_t> _sectionHeaders{};
		stringTable_t _sectionNames{};

		template<typename T> T allocate()
		{
			auto &storage{std::get<fragmentStorage_t>(_backingStorage)};
			const auto size{T::size()};
			// NOLINTNEXTLINE(modernize-avoid-c-arrays)
			const auto &allocation{storage.emplace_back(std::make_unique<uint8_t []>(size))};
			return {span{allocation.get(), size}};
		}

	public:
		elf_t(fd_t &&file);
		elf_t(class_t elfClass);

		[[nodiscard]] auto &header() noexcept { return _header; }
		[[nodiscard]] const auto &header() const noexcept { return _header; }
		[[nodiscard]] auto &programHeaders() noexcept { return _programHeaders; }
		[[nodiscard]] const auto &programHeaders() const noexcept { return _programHeaders; }
		[[nodiscard]] auto &sectionHeaders() noexcept { return _sectionHeaders; }
		[[nodiscard]] const auto &sectionHeaders() const noexcept { return _sectionHeaders; }
		[[nodiscard]] auto &sectionNames() noexcept { return _sectionNames; }
		[[nodiscard]] const auto &sectionNames() const noexcept { return _sectionNames; }
	};
} // namespace mangrove::elf

#endif /*FORMATS_ELF_HXX*/
