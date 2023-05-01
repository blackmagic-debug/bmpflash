// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef USB_CONFIGURATION_HXX
#define USB_CONFIGURATION_HXX

#include <cstdint>
#include <libusb.h>

#include "usbInterface.hxx"

struct usbConfiguration_t final
{
private:
	libusb_config_descriptor *config{nullptr};

public:
	usbConfiguration_t() noexcept = default;
	usbConfiguration_t(libusb_config_descriptor *const config_) noexcept : config{config_} { }
	~usbConfiguration_t() noexcept { libusb_free_config_descriptor(config); }

	[[nodiscard]] bool valid() const noexcept { return config; }
	[[nodiscard]] uint8_t interfaces() const noexcept { return config->bNumInterfaces; }

	[[nodiscard]] usbInterface_t interface(const size_t index) const noexcept
	{
		// If the interface requested doesn't exist, return a dummy one
		if (index >= config->bNumInterfaces)
			return {};
		// Otherwise return a real one
		return {config->interface + index};
	}
};

#endif /*USB_CONFIGURATION_HXX*/
