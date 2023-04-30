// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef USB_DEVICE_LIST_HXX
#define USB_DEVICE_LIST_HXX

#include <string_view>
#include <libusb.h>
#include <substrate/console>
#include "usbDevice.hxx"

using namespace std::literals::string_view_literals;
using substrate::console;

struct usbDeviceIter_t final
{
private:
	libusb_device **current{nullptr};

public:
	usbDeviceIter_t(libusb_device **current_) noexcept : current{current_} { }
	usbDevice_t operator *() const noexcept { return {*current}; }

	auto operator ++() noexcept
	{
		++current;
		return *this;
	}

	bool operator ==(const usbDeviceIter_t &other) const noexcept
		{ return current == other.current; }
	bool operator !=(const usbDeviceIter_t &other) const noexcept
		{ return current != other.current; }
};

struct usbDeviceList_t final
{
private:
	libusb_device **deviceList{nullptr};
	size_t count{0};

public:
	usbDeviceList_t() noexcept = default;
	usbDeviceList_t(libusb_device **const deviceList_, ssize_t count_) noexcept :
		deviceList{deviceList_}, count{static_cast<size_t>(count_)} { }

	usbDeviceList_t(const usbDeviceList_t &) noexcept = delete;
	usbDeviceList_t(usbDeviceList_t &&other) noexcept : usbDeviceList_t{} { swap(other); }
	usbDeviceList_t &operator =(const usbDeviceList_t &) noexcept = delete;

	// NOLINTNEXTLINE(modernize-use-equals-default)
	~usbDeviceList_t() noexcept
	{
		if (count)
			libusb_free_device_list(deviceList, false);
	}

	usbDeviceList_t &operator =(usbDeviceList_t &&other) noexcept
	{
		swap(other);
		return *this;
	}

	[[nodiscard]] usbDeviceIter_t begin() const noexcept { return {deviceList}; }
	[[nodiscard]] usbDeviceIter_t end() const noexcept { return {deviceList + count}; }

	void swap(usbDeviceList_t &other) noexcept
	{
		std::swap(deviceList, other.deviceList);
		std::swap(count, other.count);
	}
};

#endif /*USB_DEVICE_LIST_HXX*/
