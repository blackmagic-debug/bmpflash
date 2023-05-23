// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef SPI_FLASH_HXX
#define SPI_FLASH_HXX

#include <cstdint>
#include <cstddef>
#include <substrate/span>

struct bmp_t;

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
		sectorErase = 0x20U,
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
		const uint8_t dummyBytes, const opcode_t opcode) noexcept
	{
		return uint16_t(uint16_t(opcodeMode) | uint16_t(dataMode) |
			((uint16_t{dummyBytes} << dummyShift) & dummyMask) | uint8_t(opcode));
	}

	constexpr inline uint16_t command(const opcodeMode_t opcodeMode, const uint8_t dummyBytes,
			const opcode_t opcode) noexcept
		{ return command(opcodeMode, dataMode_t::dataIn, dummyBytes, opcode); }

	enum class command_t : uint16_t
	{
		writeEnable = command(opcodeMode_t::opcodeOnly, 0U, opcode_t::writeEnable),
		pageProgram = command(opcodeMode_t::with3BAddress, dataMode_t::dataOut, 0U, opcode_t::pageWrite),
		sectorErase = command(opcodeMode_t::with3BAddress, 0U, opcode_t::omitted),
		chipErase = command(opcodeMode_t::opcodeOnly, 0U, opcode_t::chipErase),
		readStatus = command(opcodeMode_t::opcodeOnly, dataMode_t::dataIn, 0U, opcode_t::statusRead),
		readJEDECID = command(opcodeMode_t::opcodeOnly, dataMode_t::dataIn, 0U, opcode_t::jedecID),
		readSFDP = command(opcodeMode_t::with3BAddress, dataMode_t::dataIn, 1U, opcode_t::readSFDP),
		wakeUp = command(opcodeMode_t::opcodeOnly, 0U, opcode_t::wakeUp),
	};

	constexpr inline uint8_t spiStatusBusy{1};
	constexpr inline uint8_t spiStatusWriteEnabled{2};

	struct spiFlash_t final
	{
	private:
		uint32_t pageSize_{256};
		uint32_t sectorSize_{4096};
		size_t capacity_{};
		uint8_t sectorEraseOpcode_{uint8_t(opcode_t::sectorErase)};

	public:
		constexpr spiFlash_t() noexcept = default;
		constexpr spiFlash_t(const size_t capacity) noexcept : capacity_{capacity} { }
		constexpr spiFlash_t(const size_t pageSize, const size_t sectorSize, const uint8_t sectorEraseOpcode,
			const size_t capacity) noexcept : pageSize_{static_cast<uint32_t>(pageSize)},
				sectorSize_{static_cast<uint32_t>(sectorSize)}, capacity_{capacity},
				sectorEraseOpcode_{sectorEraseOpcode} { }

		[[nodiscard]] constexpr auto valid() const noexcept { return capacity_ != 0U; }
		[[nodiscard]] constexpr auto pageSize() const noexcept { return pageSize_; }
		[[nodiscard]] constexpr auto sectorSize() const noexcept { return sectorSize_; }
		[[nodiscard]] constexpr auto capacity() const noexcept { return capacity_; }
		[[nodiscard]] constexpr auto sectorEraseOpcode() const noexcept { return sectorEraseOpcode_; }

		[[nodiscard]] bool waitFlashIdle(const bmp_t &probe);
		[[nodiscard]] bool writeBlock(const bmp_t &probe, size_t address, const substrate::span<uint8_t> &block);
	};
} // namespace bmpflash::spiFlash

#endif /*SPI_FLASH_HXX*/
