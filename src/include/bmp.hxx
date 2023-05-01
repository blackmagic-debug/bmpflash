// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef BMP_HXX
#define BMP_HXX

#include "usbDevice.hxx"

// This represents a connection to a Black Magic Probe and all the information
// needed to communicate with its GDB serial port
struct bmp_t final
{
private:
	usbDeviceHandle_t device;
	uint8_t txEndpoint{};
	uint8_t rxEndpoint{};

public:
	bmp_t(const usbDevice_t &usbDevice);
	[[nodiscard]] bool valid() const noexcept { return device.valid() && txEndpoint && rxEndpoint; }
};

#endif /*BMP_HXX*/
