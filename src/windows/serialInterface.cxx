// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include "windows/serialInterface.hxx"
#include "bmp.hxx"

[[nodiscard]] std::string serialForDevice(const usbDevice_t &device)
{
	const auto serialIndex{device.serialNumberIndex()};
	if (serialIndex == 0)
		return {};
	const auto handle{device.open()};
	return handle.readStringDescriptor(serialIndex);
}

serialInterface_t::serialInterface_t(const usbDevice_t &usbDevice)
{
}

serialInterface_t::~serialInterface_t() noexcept
{
}

void serialInterface_t::swap(serialInterface_t &interface) noexcept
{
	std::swap(device, interface.device);
}

void serialInterface_t::writePacket(const std::string_view &packet) const
{
}

std::string serialInterface_t::readPacket() const
{
	return "";
}
