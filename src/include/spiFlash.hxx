// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef SPI_FLASH_HXX
#define SPI_FLASH_HXX

#include <cstdint>
#include <cstddef>

namespace bmpflash::spiFlash
{
	struct jedecID_t
	{
		uint8_t manufacturer;
		uint8_t type;
		uint8_t capacity;
	};

	enum class opcode_t : uint8_t
	{
		omitted = 0x00U,
		jedecID = 0x9fU,
		chipErase = 0xc7U,
		blockErase = 0xd8U,
		pageRead = 0x03U,
		pageAddressRead = 0x13U,
		pageWrite = 0x02U,
		pageAddressWrite = 0x10U,
		statusRead = 0x05U,
		statusWrite = 0x01U,
		writeEnable = 0x06U,
		writeDisable = 0x04U,
		readSFDP = 0x5aU,
		wakeUp = 0xabU,
		reset = 0xffU,
	};

	enum class opcodeMode_t : uint16_t
	{
		opcodeOnly = 0U << 11U,
		with3BAddress = 1U << 11U,
	};

	enum class dataMode_t : uint16_t
	{
		dataIn = 0U << 12U,
		dataOut = 1U << 12U,
	};

	constexpr inline uint16_t opcodeMask{0x00ffU};
	constexpr inline uint16_t dummyMask{0x0700U};
	constexpr inline size_t dummyShift{8U};
	constexpr inline uint16_t opcodeModeMask{0x0800U};
	constexpr inline uint16_t dataModeMask{0x1000U};

	constexpr inline uint16_t command(const opcodeMode_t opcodeMode, const dataMode_t dataMode,
		const uint8_t dummyCycles, const opcode_t opcode) noexcept
	{
		return uint16_t(uint16_t(opcodeMode) | uint16_t(dataMode) |
			((uint16_t{dummyCycles} << dummyShift) & dummyMask) | uint8_t(opcode));
	}

	constexpr inline uint16_t command(const opcodeMode_t opcodeMode, const uint8_t dummyCycles,
			const opcode_t opcode) noexcept
		{ return command(opcodeMode, dataMode_t::dataIn, dummyCycles, opcode); }

	enum class command_t : uint16_t
	{
		writeEnable = command(opcodeMode_t::opcodeOnly, 0U, opcode_t::writeEnable),
		pageProgram = command(opcodeMode_t::with3BAddress, dataMode_t::dataOut, 0U, opcode_t::pageWrite),
		sectorErase = command(opcodeMode_t::with3BAddress, 0U, opcode_t::omitted),
		chipErase = command(opcodeMode_t::opcodeOnly, 0U, opcode_t::chipErase),
		readStatus = command(opcodeMode_t::opcodeOnly, dataMode_t::dataIn, 0U, opcode_t::statusRead),
		readJEDECID = command(opcodeMode_t::opcodeOnly, dataMode_t::dataIn, 0U, opcode_t::jedecID),
		readSFDP = command(opcodeMode_t::with3BAddress, dataMode_t::dataIn, 8U, opcode_t::readSFDP),
		wakeUp = command(opcodeMode_t::opcodeOnly, 0U, opcode_t::wakeUp),
	};
} // namespace bmpflash::spiFlash

#endif /*SPI_FLASH_HXX*/
