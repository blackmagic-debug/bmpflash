// SPDX-License-Identifier: BSD-3-Clause
#ifndef SFDP_INTERNAL_HXX
#define SFDP_INTERNAL_HXX

#include <cstdint>
#include <cstddef>
#include <array>

namespace bmpflash::sfdp
{
	enum struct accessProtocol_t : uint8_t
	{
		xspiNANDClass1 = 0xF0U,
		xspiNANDClass2 = 0xF1U,
		xspiNANDClass3WithClass1NVM = 0xF2U,
		xspiNANDClass3WithClass2NVM = 0xF3U,
		spiNANDClass1 = 0xF4U,
		spiNANDClass2 = 0xF5U,
		spiNANDClass3withClass1NVM = 0xF6U,
		spiNANDClass3withClass2NVM = 0xF7U,
		xspiNORProfile2 = 0xFAU,
		xspiNORProfile1with3ByteAddr = 0xFCU,
		xspiNORProfile1with4ByteAddr20Wait = 0xFDU,
		xspiNORProfile1with4ByteAddr8Wait = 0xFEU,
		legacyJESD216B = 0xFFU
	};

	struct uint24_t
	{
	private:
		std::array<uint8_t, 3> value{};

	public:
		operator uint32_t() const noexcept
			{ return static_cast<uint32_t>((value[2] << 16U) | (value[1] << 8U) | value[0]); }
	};

// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
	struct sfdpHeader_t
	{
		std::array<char, 4> magic{};
		uint8_t versionMajor{};
		uint8_t versionMinor{};
		uint8_t rawParameterHeadersCount{};
		accessProtocol_t accessProtocol{accessProtocol_t::legacyJESD216B};

		[[nodiscard]] size_t parameterHeadersCount() const noexcept { return rawParameterHeadersCount + 1U; }
	};

	struct parameterTableHeader_t
	{
		uint8_t jedecParameterIDLow{};
		uint8_t versionMajor{};
		uint8_t versionMinor{};
		uint8_t tableLengthInU32s{};
		uint24_t tableAddress{};
		uint8_t jedecParameterIDHigh{};

		[[nodiscard]] uint16_t jedecParameterID() const noexcept
			{ return static_cast<uint16_t>((jedecParameterIDHigh << 8U) | jedecParameterIDLow); }
		[[nodiscard]] size_t tableLength() const noexcept { return static_cast<size_t>(tableLengthInU32s) * 4U; }
	};

	struct memoryDensity_t
	{
		std::array<uint8_t, 4> data{};

	private:
		[[nodiscard]] bool isExponential() const noexcept { return data[3] & 0x80U; }
		[[nodiscard]] uint32_t value() const noexcept
			{ return uint32_t((data[3] & 0x7fU) << 24U) | uint32_t(data[2] << 16U) | uint32_t(data[1] << 8U) | data[0]; }

	public:
		[[nodiscard]] size_t capacity() const noexcept
		{
			const auto bits
			{
				[=]() -> size_t
				{
					if (isExponential())
						return UINT64_C(1) << value();
					return value() + 1U;
				}()
			};
			return bits / 8U;
		}
	};

	struct [[gnu::packed]] timingsAndOpcode_t
	{
		uint8_t timings{};
		uint8_t opcode{};
	};

	struct [[gnu::packed]] eraseParameters_t
	{
		uint8_t eraseSizeExponent{};
		uint8_t opcode{};

		[[nodiscard]] size_t eraseSize() const noexcept
			{ return UINT64_C(1) << eraseSizeExponent; }
	};

	struct programmingAndChipEraseTiming_t
	{
		uint8_t programmingTimingRatioAndPageSize{};
		std::array<uint8_t, 3> eraseTimings;

		[[nodiscard]] size_t pageSize() const noexcept
		{
			const uint8_t pageSizeExponent = programmingTimingRatioAndPageSize >> 4U;
			return UINT64_C(1) << pageSizeExponent;
		}
	};

	struct deepPowerDown_t
	{
		std::array<uint8_t, 3> data{};

		[[nodiscard]] uint8_t enterInstruction() const noexcept
		{
			const auto value{(uint16_t{data[2]} << 8U) | uint16_t{data[1]}};
			return uint8_t(value >> 7U);
		}

		[[nodiscard]] uint8_t exitInstruction() const noexcept
		{
			const auto value{(uint16_t{data[1]} << 8U) | uint16_t{data[0]}};
			return uint8_t(value >> 7U);
		}
	};

	struct basicParameterTable_t
	{
		uint8_t value1{};
		uint8_t sectorEraseOpcode{};
		uint8_t value2{};
		uint8_t reserved1{};
		memoryDensity_t flashMemoryDensity{};
		timingsAndOpcode_t fastQuadIO{};
		timingsAndOpcode_t fastQuadOutput{};
		timingsAndOpcode_t fastDualOutput{};
		timingsAndOpcode_t fastDualIO{};
		uint8_t fastSupportFlags{};
		std::array<uint8_t, 5> reserved2{};
		timingsAndOpcode_t fastDualDPI{};
		std::array<uint8_t, 2> reserved3{};
		timingsAndOpcode_t fastQuadQPI{};
		std::array<eraseParameters_t, 4> eraseTypes{};
		uint32_t eraseTiming{};
		programmingAndChipEraseTiming_t programmingAndChipEraseTiming{};
		uint8_t operationalProhibitions{};
		std::array<uint8_t, 3> suspendLatencySpecs{};
		uint8_t programResumeOpcode{};
		uint8_t programSuspendOpcode{};
		uint8_t resumeOpcode{};
		uint8_t suspendOpcode{};
		uint8_t statusRegisterPollingFlags{};
		deepPowerDown_t deepPowerdown{};
		std::array<uint8_t, 3> dualAndQuadMode{};
		uint8_t reserved4{};
		uint32_t statusAndAddressingMode{};
	};
// NOLINTEND(misc-non-private-member-variables-in-classes)

	static_assert(sizeof(uint24_t) == 3);
	static_assert(sizeof(sfdpHeader_t) == 8);
	static_assert(sizeof(parameterTableHeader_t) == 8);
	static_assert(sizeof(memoryDensity_t) == 4);
	static_assert(sizeof(timingsAndOpcode_t) == 2);
	static_assert(sizeof(programmingAndChipEraseTiming_t) == 4);
	static_assert(sizeof(basicParameterTable_t) == 64);
} // namespace bmpflash::sfdp

#endif /*SFDP_INTERNAL_HXX*/
