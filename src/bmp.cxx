// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <substrate/index_sequence>
#include "bmp.hxx"

using usb::descriptors::usbClass_t;

bmp_t::bmp_t(const usbDevice_t &usbDevice) : device{usbDevice.open()}
{
	// To figure out the endpoints for the GDB serial port, first grab the active configuration
	const auto config{usbDevice.activeConfiguration()};
	if (!config.valid())
		return;
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
		if (firstAltMode.interfaceClass() != usbClass_t::cdcACM)
			continue;
	}
}
