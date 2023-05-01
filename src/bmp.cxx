// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <substrate/console>
#include <substrate/index_sequence>
#include <substrate/span>
#include "bmp.hxx"

using substrate::console;
using usb::descriptors::usbClass_t;
using cdcCommsSubclass_t = usb::descriptors::subclasses::cdcComms_t;
using cdcCommsProtocol_t = usb::descriptors::protocols::cdcComms_t;
using usb::descriptors::cdc::functionalDescriptor_t;
using usb::descriptors::cdc::callManagementDescriptor_t;

template<typename descriptor_t> auto descriptorFromSpan(const substrate::span<const uint8_t> &data)
{
	descriptor_t result{};
	// Check there's at least enough data to fill the structure
	if (data.size() < sizeof(result))
		return result;
	// Copy the necessary data and return it
	std::memcpy(&result, data.data(), sizeof(result));
	return result;
}

uint8_t locateDataInterface(const substrate::span<const uint8_t> &descriptorData)
{
	using usb::descriptors::cdc::descriptorType_t;
	using usb::descriptors::cdc::descriptorSubtype_t;
	size_t offset{};
	// Iterate through the descriptor data
	for (; offset < descriptorData.size(); )
	{
		// Safely unpack the next descriptor
		auto descriptor{descriptorFromSpan<functionalDescriptor_t>(descriptorData.subspan(offset))};
		// Check if it's a call management descriptor
		if (descriptor.length == sizeof(callManagementDescriptor_t) &&
			descriptor.type == descriptorType_t::interface &&
			descriptor.subtype == descriptorSubtype_t::callManagement)
		{
			// Try unpacking the descriptor
			auto callManagement{descriptorFromSpan<callManagementDescriptor_t>(descriptorData.subspan(offset))};
			// If the length is 0, unpacking failed so bail out
			if (!callManagement.length)
				break;
			// Otherwise, we've got our data interface!
			return callManagement.dataInterface;
		}
		// Try to increment the offset by the length and check the length's not 0
		// (if it is, unpacking failed so we should bail out)
		offset += descriptor.length;
		if (!descriptor.length)
			break;
	}
	return UINT8_MAX;
}

bmp_t::bmp_t(const usbDevice_t &usbDevice) : device{usbDevice.open()}
{
	// To figure out the endpoints for the GDB serial port, first grab the active configuration
	const auto config{usbDevice.activeConfiguration()};
	if (!config.valid())
		return;
	uint8_t dataIfaceIndex{UINT8_MAX};
	// Then iterate through the interfaces it defines
	for (const auto idx : substrate::indexSequence_t{config.interfaces()})
	{
		// Get each interface and inspect the first alt-mode
		const auto interface{config.interface(idx)};
		if (!interface.valid())
			return;
		const auto firstAltMode{interface.altMode(0)};
		if (!firstAltMode.valid())
			return;

		// Look for interfaces implementing CDC ACM
		if (firstAltMode.interfaceClass() != usbClass_t::cdcComms ||
			firstAltMode.interfaceSubClass<cdcCommsSubclass_t>() != cdcCommsSubclass_t::abstractControl ||
			firstAltMode.interfaceProtocol<cdcCommsProtocol_t>() != cdcCommsProtocol_t::none)
			continue;

		// Now grab the interface description string and check it matches the GDB server interface string
		const auto ifaceName{device.readStringDescriptor(firstAltMode.interfaceIndex())};
		if (ifaceName != "Black Magic GDB Server"sv)
			continue;

		dataIfaceIndex = locateDataInterface(firstAltMode.extraDescriptors());
		break;
	}
	// Check for errors finding the data interface
	if (dataIfaceIndex == UINT8_MAX)
	{
		console.error("Failed to find GDB server data interface"sv);
		return;
	}
}
