// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef PROVISION_ELF_HXX
#define PROVISION_ELF_HXX

#include <filesystem>
#include "bmp.hxx"
#include "elf/elf.hxx"

namespace bmpflash::elf
{
	using std::filesystem::path;

	struct provision_t final
	{
	private:
		elf_t file;

	public:
		provision_t(const path &fileName) noexcept;

		[[nodiscard]] bool valid() const noexcept;
		[[nodiscard]] bool repack(const bmp_t &probe) const;
	};
} // namespace bmpflash::elf

#endif /*PROVISION_ELF_HXX*/
