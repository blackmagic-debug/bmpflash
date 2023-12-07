// SPDX-License-Identifier: BSD-3-Clause
#ifndef SFDP_HXX
#define SFDP_HXX

#include <optional>
#include "spiFlash.hxx"
#include "bmp.hxx"

namespace bmpflash::sfdp
{
	using bmpflash::spiFlash::spiFlash_t;

	bool readAndDisplay(const bmp_t &probe, bool displayRaw);
	std::optional<spiFlash_t> read(const bmp_t &probe);
} // namespace bmpflash::sfdp

#endif /*SFDP_HXX*/
