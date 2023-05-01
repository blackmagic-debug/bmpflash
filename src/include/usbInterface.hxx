// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef USB_INTERFACE_HXX
#define USB_INTERFACE_HXX

#include <cstddef>
#include <libusb.h>

struct usbInterfaceAltMode_t final
{
private:
	const libusb_interface_descriptor *interface{nullptr};

public:
	usbInterfaceAltMode_t() noexcept = default;
	usbInterfaceAltMode_t(const libusb_interface_descriptor *const iface) noexcept : interface{iface} { }

	[[nodiscard]] bool valid() const noexcept { return interface; }
	[[nodiscard]] uint8_t endpoints() const noexcept { return interface->bNumEndpoints; }
	[[nodiscard]] uint8_t interfaceClass() const noexcept { return interface->bInterfaceClass; }
	[[nodiscard]] uint8_t interfaceSubClass() const noexcept { return interface->bInterfaceSubClass; }
	[[nodiscard]] uint8_t interfaceProtocol() const noexcept { return interface->bInterfaceProtocol; }
	[[nodiscard]] auto interfaceIndex() const noexcept { return interface->iInterface; }
};

struct usbInterface_t final
{
private:
	const libusb_interface *interface{nullptr};

public:
	usbInterface_t() noexcept = default;
	usbInterface_t(const libusb_interface *const iface) noexcept : interface{iface} { }

	[[nodiscard]] bool valid() const noexcept { return interface; }
	[[nodiscard]] auto altModes() const noexcept { return static_cast<size_t>(interface->num_altsetting); }

	[[nodiscard]] usbInterfaceAltMode_t altMode(const size_t index) const noexcept
	{
		// If the alt-mode requested doesn't exist, return a dummy one
		if (index >= altModes())
			return {};
		// Otherwise return a real one
		return {interface->altsetting + index};
	}
};

#endif /*USB_INTERFACE_HXX*/
