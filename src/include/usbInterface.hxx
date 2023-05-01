// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef USB_INTERFACE_HXX
#define USB_INTERFACE_HXX

#include <cstddef>
#include <libusb.h>

struct usbInterface_t final
{
private:
	const libusb_interface *interface{nullptr};

public:
	usbInterface_t() noexcept = default;
	usbInterface_t(const libusb_interface *const iface) noexcept : interface{iface} { }

	[[nodiscard]] bool valid() const noexcept { return interface; }
	[[nodiscard]] auto altModes() const noexcept { return static_cast<size_t>(interface->num_altsetting); }
};

#endif /*USB_INTERFACE_HXX*/
