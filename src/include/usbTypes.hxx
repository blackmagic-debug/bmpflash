// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef USB_TYPES_HXX
#define USB_TYPES_HXX

#include <cstdint>

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

#endif /*USB_TYPES_HXX*/
