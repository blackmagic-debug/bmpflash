// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <substrate/console>
#include <substrate/index_sequence>
#include "bmp.hxx"

using substrate::console;
using usb::descriptors::usbClass_t;
using cdcCommsSubclass_t = usb::descriptors::subclasses::cdcComms_t;
using cdcCommsProtocol_t = usb::descriptors::protocols::cdcComms_t;

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

		//dataIfaceIndex = ;
		break;
	}
	// Check for errors finding the data interface
	if (dataIfaceIndex == UINT8_MAX)
	{
		console.error("Failed to find GDB server data interface"sv);
		return;
	}
}
