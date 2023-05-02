// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef USB_TYPES_HXX
#define USB_TYPES_HXX

#include <cstdint>

enum class endpointDir_t : uint8_t
{
	controllerOut = 0x00U,
	controllerIn = 0x80U
};

constexpr static const uint8_t endpointDirMask{0x7fU};
constexpr inline uint8_t endpointAddress(const endpointDir_t dir, const uint8_t number) noexcept
	{ return uint8_t(dir) | (number & endpointDirMask); }

// This code is borrowed from dragonUSB
namespace usb::descriptors
{
	enum class usbClass_t : uint8_t
	{
		none = 0x00U,
		audio = 0x01U,
		cdcComms = 0x02U,
		hid = 0x03U,
		physical = 0x05U,
		image = 0x06U,
		printer = 0x07U,
		massStorage = 0x08U,
		hub = 0x09U,
		cdcData = 0x0AU,
		smartCard = 0x0BU,
		contentSecurity = 0x0DU,
		video = 0x0EU,
		healthcare = 0x0FU,
		audioVisual = 0x10U,
		billboard = 0x11U,
		typeCBridge = 0x12U,
		diagnostic = 0xDCU,
		wireless = 0xE0U,
		misc = 0xEFU,
		application = 0xFEU,
		vendor = 0xFFU
	};

	namespace subclasses
	{
		enum class cdcComms_t : uint8_t
		{
			directLineControl = 1,
			abstractControl = 2,
			telephoneControl = 3,
			multiChannelControl = 4,
			capiControl = 5,
			ethernetNetworkingControl = 6,
			atmNetworkingControl = 7,
		};
	} // namespace subclasses

	namespace protocols
	{
		enum class cdcComms_t : uint8_t
		{
			none = 0,
			v25ter = 1,
			vendor = 255,
		};
	} // namespace protocols

	namespace cdc
	{
		enum class descriptorType_t : uint8_t
		{
			interface = 0x24U,
			endpoint = 0x25U,
		};

		enum class descriptorSubtype_t : uint8_t
		{
			header = 0U,
			callManagement = 1U,
			abstractControlManagement = 2U,
			directLineManagement = 3U,
			telephoneRinger = 4U,
			telephoneCapabilities = 5U,
			interfaceUnion = 6U,
			countrySelection = 7U,
			telephoneOperational = 8U,
			usbTerminal = 9U,
			networkChannel = 10U,
			protocolUnit = 11U,
			extensionUnit = 12U,
			multiChannelManagement = 13U,
			capiControlManagement = 14U,
			ethernetNetworking = 15U,
			atmNetworking = 16U,
		};

		struct functionalDescriptor_t final
		{
			uint8_t length;
			descriptorType_t type;
			descriptorSubtype_t subtype;
		};

		struct [[gnu::packed]] headerDescriptor_t final
		{
			uint8_t length;
			descriptorType_t type;
			descriptorSubtype_t subtype;
			uint16_t cdcVersion;
		};

		struct callManagementDescriptor_t final
		{
			uint8_t length;
			descriptorType_t type;
			descriptorSubtype_t subtype;
			uint8_t capabilities;
			uint8_t dataInterface;
		};
	} // namespace cdc
} // namespace usb::descriptors

namespace usb::types::cdc
{
	enum class request_t : uint8_t
	{
		sendEncapsulatedCommand = 0x00U,
		getEncapsulatedResponse = 0x01U,
		setCommFeature = 0x02U,
		getCommFeature = 0x03U,
		clearCommFeature = 0x04U,

		setAuxLineState = 0x10U,
		setHookState = 0x11U,
		pulseSetup = 0x12U,
		sendPulse = 0x13U,
		setPulseTime = 0x14U,
		ringAuxJack = 0x15U,

		setLineCoding = 0x20U,
		getLineCoding = 0x21U,
		setControlLineState = 0x22U,
		sendBreak = 0x23U,

		setRingerParams = 0x30U,
		getRingerParams = 0x31U,
		setOperationParmas = 0x32U,
		getOperatoinParams = 0x33U,
		setLineParams = 0x34U,
		getLineParams = 0x35U,
		dialDigits = 0x36U,
		setUnitParameter = 0x37U,
		getUnitParameter = 0x38U,
		clearUnitParameter = 0x39U,
		getProfile = 0x3aU,

		setEthernetMulticastFilters = 0x40U,
		setEthernetPowerManagementPattern = 0x41U,
		getEthernetPowerManagementPattern = 0x42U,
		setEthernetPacketFilter = 0x43U,
		getEthernetStatistic = 0x44U,

		setATMDataFormat = 0x50U,
		getATMDeviceStatistics = 0x51U,
		setATMDefaultVC = 0x52U,
		getATMVCStatistics = 0x53U,
	};

	enum class controlLines_t : uint16_t
	{
		dtrPresent = 1U,
		rtsActivate = 2U,
	};

	constexpr inline uint16_t operator |(const controlLines_t a, const controlLines_t b) noexcept
		{ return uint16_t(a) | uint16_t(b); }
} // namespace usb::types::cdc

#endif /*USB_TYPES_HXX*/
