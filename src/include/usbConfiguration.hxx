// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef USB_CONFIGURATION_HXX
#define USB_CONFIGURATION_HXX

#include <libusb.h>

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
};

#endif /*USB_CONFIGURATION_HXX*/
