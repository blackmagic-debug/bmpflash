// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef SERIAL_INTERFACE_HXX
#define SERIAL_INTERFACE_HXX

#include "usbDevice.hxx"

struct serialInterface_t
{
private:
	usbDeviceHandle_t device{};
	uint8_t ctrlInterfaceNumber{UINT8_MAX};
	uint8_t dataInterfaceNumber{UINT8_MAX};
	uint8_t txEndpoint{};
	uint8_t rxEndpoint{};

public:
	serialInterface_t() noexcept = default;
	serialInterface_t(const usbDevice_t &usbDevice);
	serialInterface_t(const serialInterface_t &) noexcept = delete;
	serialInterface_t(serialInterface_t &&probe) noexcept : serialInterface_t{} { swap(probe); }
	~serialInterface_t() noexcept;
	serialInterface_t &operator =(const serialInterface_t &) noexcept = delete;

	serialInterface_t &operator =(serialInterface_t &&interface) noexcept
	{
		swap(interface);
		return *this;
	}

	[[nodiscard]] bool valid() const noexcept { return device.valid() && txEndpoint && rxEndpoint; }
	void swap(serialInterface_t &interface) noexcept;

	void writePacket(const std::string_view &packet) const;
	[[nodiscard]] std::string readPacket() const;
};

#endif /*SERIAL_INTERFACE_HXX*/
