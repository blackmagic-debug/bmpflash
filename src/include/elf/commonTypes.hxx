// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 The Mangrove Language
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef ELF_COMMON_TYPES_HXX
#define ELF_COMMON_TYPES_HXX

#include "io.hxx"
#include "enums.hxx"

namespace bmpflash::elf::types
{
	inline namespace internal
	{
		using bmpflash::elf::io::memory_t;
		using namespace bmpflash::elf::enums;
	} // namespace internal

	// This represents the magic number \x7f ELF
	constexpr static inline std::array<uint8_t, 4> elfMagic{{0x7fU, 0x45U, 0x4cU, 0x46U}};

	struct elfIdent_t
	{
	protected:
		// NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
		memory_t _storage;
		// NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
		endian_t _endian;

	public:
		elfIdent_t(const memory_t &storage) : _storage{storage}, _endian{_storage.read<endian_t>(5)} {}

		[[nodiscard]] auto magic() const noexcept { return _storage.read<std::array<uint8_t, 4>>(0); }
		[[nodiscard]] auto elfClass() const noexcept { return _storage.read<class_t>(4); }
		[[nodiscard]] auto endian() const noexcept { return _endian; }
		[[nodiscard]] auto version() const noexcept { return _storage.read<identVersion_t>(6); }
		[[nodiscard]] auto abi() const noexcept { return _storage.read<abi_t>(7); }
		[[nodiscard]] auto padding() const noexcept { return _storage.read<std::array<uint8_t, 8>>(8); }

		[[nodiscard]] constexpr static size_t size() noexcept { return 16U; }
	};
} // namespace bmpflash::elf::types

#endif /*ELF_COMMON_TYPES_HXX*/
