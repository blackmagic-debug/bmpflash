// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef USB_CONTEXT_HXX
#define USB_CONTEXT_HXX

#ifndef __STDC_VERSION__
// Horrible hack to make libusb conformant and not do stupid things.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cppcoreguidelines-macro-usage,cert-dcl37-c,cert-dcl51-cpp)
#define __STDC_VERSION__ 199901L
#endif

#include <string_view>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <libusb.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include <substrate/console>

#include "usbDeviceList.hxx"

using namespace std::literals::string_view_literals;
using substrate::console;

struct usbContext_t final
{
private:
	libusb_context *context{nullptr};

public:
	usbContext_t() noexcept
	{
		if (auto result = libusb_init(&context); result)
		{
			console.error("Failed to initialise libusb-1.0: "sv, libusb_error_name(result));
			context = nullptr;
		}
	}

	usbContext_t(const usbContext_t &) noexcept = delete;
	usbContext_t(usbContext_t &&other) noexcept : usbContext_t{} { swap(other); }
	usbContext_t &operator =(const usbContext_t &) noexcept = delete;

	// NOLINTNEXTLINE(modernize-use-equals-default)
	~usbContext_t() noexcept
	{
		if (context)
			libusb_exit(context);
	}

	usbContext_t &operator =(usbContext_t &&other) noexcept
	{
		swap(other);
		return *this;
	}

	[[nodiscard]] auto valid() const noexcept { return context != nullptr; }

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] usbDeviceList_t deviceList() const noexcept
	{
		libusb_device **list{nullptr};
		auto result = libusb_get_device_list(context, &list);
		if (result < 0)
		{
			console.error("Failed to get device list: "sv, libusb_error_name(static_cast<int>(result)));
			return {};
		}
		return {list, result};
	}

	void swap(usbContext_t &other) noexcept
		{ std::swap(context, other.context); }
};

#endif /*USB_CONSTEXT_HXX*/
