// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef BMP_HXX
#define BMP_HXX

#include <string>
#include <string_view>
#include <exception>
#include "usbDevice.hxx"

struct bmpCommsError_t final : std::exception
{
	[[nodiscard]] const char *what() const noexcept final;
};

enum class spiDevice_t : uint8_t
{
	intFlash = 0,
	extFlash = 1,
	sdcard = 2,
	display = 3,
	none = 255,
};

enum class spiBus_t : uint8_t
{
	external = 0,
	internal = 1,
	none = 255,
};

// This represents a connection to a Black Magic Probe and all the information
// needed to communicate with its GDB serial port
struct bmp_t final
{
private:
	usbDeviceHandle_t device;
	uint8_t ctrlInterfaceNumber{UINT8_MAX};
	uint8_t dataInterfaceNumber{UINT8_MAX};
	uint8_t txEndpoint{};
	uint8_t rxEndpoint{};
	spiBus_t spiBus{spiBus_t::none};
	constexpr static size_t maxPacketSize{1024U};

	void writePacket(const std::string_view &packet) const;
	[[nodiscard]] std::string readPacket() const;

public:
	bmp_t(const usbDevice_t &usbDevice);
	~bmp_t() noexcept;
	[[nodiscard]] bool valid() const noexcept { return device.valid() && txEndpoint && rxEndpoint; }

	[[nodiscard]] std::string init() const;
	[[nodiscard]] uint64_t readProtocolVersion() const;
	[[nodiscard]] bool begin(spiBus_t bus) noexcept;
	[[nodiscard]] bool end() noexcept;
};

#endif /*BMP_HXX*/
