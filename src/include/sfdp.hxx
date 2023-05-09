// SPDX-License-Identifier: BSD-3-Clause
#ifndef SFDP_HXX
#define SFDP_HXX

#include "bmp.hxx"

namespace bmpflash::sfdp
{
	bool readAndDisplay(const bmp_t &probe);
} // namespace bmpflash::sfdp

#endif /*SFDP_HXX*/
