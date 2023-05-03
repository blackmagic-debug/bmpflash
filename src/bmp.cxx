// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <cstring>
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
using cdcRequest_t = usb::types::cdc::request_t;
using usb::types::cdc::controlLines_t;

template<typename descriptor_t> auto descriptorFromSpan(const substrate::span<const uint8_t> &data) noexcept
{
	descriptor_t result{};
	// Check there's at least enough data to fill the structure
	if (data.size() < sizeof(result))
		return result;
	// Copy the necessary data and return it
	std::memcpy(&result, data.data(), sizeof(result));
	return result;
}

uint8_t locateDataInterface(const substrate::span<const uint8_t> &descriptorData) noexcept
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
			console.debug("Found CDC Call Management descriptor in extra data at +"sv, offset);
			// Try unpacking the descriptor
			auto callManagement{descriptorFromSpan<callManagementDescriptor_t>(descriptorData.subspan(offset))};
			// If the length is 0, unpacking failed so bail out
			if (!callManagement.length)
				break;
			console.debug("Found GDB server data interface number: "sv, callManagement.dataInterface);
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

std::pair<uint8_t, uint8_t> extractGDBInterface(const usbDeviceHandle_t &device, const usbConfiguration_t &config)
{
	// Iterate through the interfaces the configuration defines
	for (const auto idx : substrate::indexSequence_t{config.interfaces()})
	{
		// Get each interface and inspect the first alt-mode
		const auto interface{config.interface(idx)};
		if (!interface.valid())
			break;
		const auto firstAltMode{interface.altMode(0)};
		if (!firstAltMode.valid())
			break;

		// Look for interfaces implementing CDC ACM
		if (firstAltMode.interfaceClass() != usbClass_t::cdcComms ||
			firstAltMode.interfaceSubClass<cdcCommsSubclass_t>() != cdcCommsSubclass_t::abstractControl ||
			firstAltMode.interfaceProtocol<cdcCommsProtocol_t>() != cdcCommsProtocol_t::none)
			continue;

		// Now grab the interface description string and check it matches the GDB server interface string
		const auto ifaceName{device.readStringDescriptor(firstAltMode.interfaceIndex())};
		console.debug("Found CDC ACM interface: "sv, ifaceName);
		if (ifaceName != "Black Magic GDB Server"sv)
			continue;
		console.debug("Found GDB server interface at index "sv, idx, " ("sv, firstAltMode.interfaceNumber(), ')');

		// Found it! Now parse the CDC functional descriptors that follow to find the data interface number
		return {firstAltMode.interfaceNumber(), locateDataInterface(firstAltMode.extraDescriptors())};
	}
	return {UINT8_MAX, UINT8_MAX};
}

bmp_t::bmp_t(const usbDevice_t &usbDevice) : device{usbDevice.open()}
{
	// To figure out the endpoints for the GDB serial port, first grab the active configuration
	const auto config{usbDevice.activeConfiguration()};
	if (!config.valid())
		return;
	// Then hunt through the descriptors looking for the data interface number
	std::tie(ctrlInterfaceNumber, dataInterfaceNumber) = extractGDBInterface(device, config);
	// Check for errors finding the data interface
	if (dataInterfaceNumber == UINT8_MAX)
	{
		console.error("Failed to find GDB server data interface"sv);
		return;
	}

	// Re-iterate the interface list to find the data interface
	for (const auto idx : substrate::indexSequence_t{config.interfaces()})
	{
		// Get each interface and inspect the first alt-mode
		const auto interface{config.interface(idx)};
		const auto firstAltMode{interface.altMode(0)};

		// Check if the interface matches the data interface index
		if (firstAltMode.interfaceNumber() != dataInterfaceNumber)
			continue;

		// We've got a match, so now check how many endpoints are reported
		if (firstAltMode.endpoints() != 2U)
		{
			console.error("Probe descriptors are invalid"sv);
			return;
		}
		// And iterate through them to extract the addresses
		for (const auto epIndex : substrate::indexSequence_t{2U})
		{
			const auto endpoint{firstAltMode.endpoint(epIndex)};
			if (endpoint.direction() == endpointDir_t::controllerOut)
				txEndpoint = endpoint.address();
			else
				rxEndpoint = endpoint.address();
		}
		break;
	}
	// Validate the endpoint IDs and claim the interface, then, having claimed the interface
	// ask it to become active by sending a SET_CONTROL_LINE_STATE control request via
	// the associated CDC control interface}
	if (!txEndpoint || !rxEndpoint || !device.claimInterface(ctrlInterfaceNumber) || !device.claimInterface(dataInterfaceNumber) ||
		!device.writeControl({recipient_t::interface, request_t::typeClass}, uint8_t(cdcRequest_t::setControlLineState),
			controlLines_t::dtrPresent | controlLines_t::rtsActivate, ctrlInterfaceNumber, nullptr))
	{
		// If either of them are bad, we couldn't claim the interface, or we couldn't send the control request, invalidate both.
		txEndpoint = 0U;
		rxEndpoint = 0U;
	}
	// Having adjusted the line state, try to do a read of the serial state notification which will be sat in the buffer
	std::array<uint8_t, 10U> serialState{};
	static_cast<void>(device.readBulk(rxEndpoint, serialState.data(), serialState.size(), 100ms));
}

bmp_t::~bmp_t() noexcept
{
	if (ctrlInterfaceNumber != UINT8_MAX)
		// Send a SET_CONTROL_LINE_STATE control request to reset the interface
		static_cast<void>(device.writeControl({recipient_t::interface, request_t::typeClass},
			uint8_t(cdcRequest_t::setControlLineState), 0U, ctrlInterfaceNumber, nullptr));
	if (dataInterfaceNumber != UINT8_MAX)
		// Release the data interface
		static_cast<void>(device.releaseInterface(dataInterfaceNumber));
	if (ctrlInterfaceNumber != UINT8_MAX)
		// Release the data interface
		static_cast<void>(device.releaseInterface(ctrlInterfaceNumber));
}

void bmp_t::writePacket(const std::string_view &packet) const
{
	console.debug("Remote write: "sv, packet);
	if (!device.writeBulk(txEndpoint, packet.data(), static_cast<int32_t>(packet.length())))
		throw bmpCommsError_t{};
}

std::string bmp_t::readPacket() const
{
	std::array<char, maxPacketSize + 1U> packet{};
	// Read back what we can and check we got a valid response packet
	if (!device.readBulk(rxEndpoint, packet.data(), maxPacketSize) || packet[0] != '&')
		throw bmpCommsError_t{};
	// Figure out how long that is
	const auto length{std::strlen(packet.data() + 1U)};
	// Make a new std::string of an appropriate length
	std::string result(length + 1U, '\0');
	// And copy the result string in, returning it
	std::memcpy(result.data(), packet.data() + 1U, length);
	return result;
}

const char *bmpCommsError_t::what() const noexcept
{
	return "Communications failure with Black Magic Probe";
}
