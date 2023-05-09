// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef SPI_FLASH_HXX
#define SPI_FLASH_HXX

#include <cstdint>

struct spiFlashID_t
{
	uint8_t manufacturer;
	uint8_t type;
	uint8_t capacity;
};

#endif /*SPI_FLASH_HXX*/
