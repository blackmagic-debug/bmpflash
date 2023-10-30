// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include "bmp.hxx"

bmp_t::bmp_t(const usbDevice_t &usbDevice) : device{usbDevice} { }

bmp_t::~bmp_t() noexcept
{
	if (_spiBus != spiBus_t::none)
		static_cast<void>(end());
}

void bmp_t::swap(bmp_t &probe) noexcept
{
	device.swap(probe.device);
	std::swap(_spiBus, probe._spiBus);
	std::swap(_spiDevice, probe._spiDevice);
}

const char *bmpCommsError_t::what() const noexcept
{
	return "Communications failure with Black Magic Probe";
}
