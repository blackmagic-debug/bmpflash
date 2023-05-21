// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef USB_INTERFACE_HXX
#define USB_INTERFACE_HXX

#include <cstddef>
#include <libusb.h>
#include <substrate/span>

#include "usbEndpoint.hxx"
#include "usbTypes.hxx"

struct usbInterfaceAltMode_t final
{
private:
	const libusb_interface_descriptor *interface{nullptr};

public:
	usbInterfaceAltMode_t() noexcept = default;
	usbInterfaceAltMode_t(const libusb_interface_descriptor *const iface) noexcept : interface{iface} { }

	[[nodiscard]] bool valid() const noexcept { return interface; }
	[[nodiscard]] uint8_t endpoints() const noexcept { return interface->bNumEndpoints; }
	[[nodiscard]] uint8_t interfaceNumber() const noexcept { return interface->bInterfaceNumber; }
	[[nodiscard]] auto interfaceClass() const noexcept
		{ return static_cast<usb::descriptors::usbClass_t>(interface->bInterfaceClass); }
	template<typename T> [[nodiscard]] auto interfaceSubClass() const noexcept
		{ return static_cast<T>(interface->bInterfaceSubClass); }
	template<typename T> [[nodiscard]] auto interfaceProtocol() const noexcept
		{ return static_cast<T>(interface->bInterfaceProtocol); }
	[[nodiscard]] auto interfaceIndex() const noexcept { return interface->iInterface; }

	// NOTLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] usbEndpoint_t endpoint(const size_t index) const noexcept
	{
		// If the endpoint requested doesn't exist, return a dummy one
		if (index >= interface->bNumEndpoints)
			return {};
		// Otherwise return a real one
		return {interface->endpoint + index};
	}

	[[nodiscard]] substrate::span<const uint8_t> extraDescriptors() const noexcept
		{ return {interface->extra, static_cast<size_t>(interface->extra_length)}; }
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
