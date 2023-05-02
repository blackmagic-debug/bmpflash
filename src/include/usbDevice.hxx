// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef USB_DEVICE_HXX
#define USB_DEVICE_HXX

#ifndef __STDC_VERSION__
// Horrible hack to make libusb conformant and not do stupid things.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cppcoreguidelines-macro-usage,cert-dcl37-c,cert-dcl51-cpp)
#define __STDC_VERSION__ 199901L
#endif

#include <cassert>
#include <string_view>
#include <utility>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <libusb.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include <substrate/console>

#include "unicode.hxx"
#include "usbConfiguration.hxx"
#include "usbTypes.hxx"

using namespace std::literals::string_view_literals;
using substrate::console;
using substrate::asHex_t;

enum class request_t : uint8_t
{
	typeStandard = 0x00,
	typeClass = 0x20U,
	typeVendor = 0x40U
};

enum class recipient_t : uint8_t
{
	device = 0,
	interface = 1,
	endpoint = 2,
	other = 3
};

struct requestType_t final
{
private:
	uint8_t value{};

public:
	constexpr requestType_t() noexcept = default;
	constexpr requestType_t(const recipient_t recipient, const request_t type) noexcept :
		requestType_t{recipient, type, endpointDir_t::controllerOut} { }
	constexpr requestType_t(const recipient_t recipient, const request_t type, const endpointDir_t direction) noexcept :
		value(static_cast<uint8_t>(recipient) | static_cast<uint8_t>(type) | static_cast<uint8_t>(direction)) { }

	void recipient(const recipient_t recipient) noexcept
	{
		value &= 0xE0U;
		value |= static_cast<uint8_t>(recipient);
	}

	void type(const request_t type) noexcept
	{
		value &= 0x9FU;
		value |= static_cast<uint8_t>(type);
	}

	void dir(const endpointDir_t direction) noexcept
	{
		value &= 0x7FU;
		value |= static_cast<uint8_t>(direction);
	}

	[[nodiscard]] recipient_t recipient() const noexcept
		{ return static_cast<recipient_t>(value & 0x1FU); }
	[[nodiscard]] request_t type() const noexcept
		{ return static_cast<request_t>(value & 0x60U); }
	[[nodiscard]] endpointDir_t dir() const noexcept
		{ return static_cast<endpointDir_t>(value & 0x80U); }
	[[nodiscard]] operator uint8_t() const noexcept { return value; }
};

struct usbDeviceHandle_t final
{
private:
	libusb_device_handle *device{nullptr};

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] bool interruptTransfer(const uint8_t endpoint, const void *const bufferPtr,
		const int32_t bufferLen) const noexcept
	{
		// The const-cast here is required becasue libusb is not const-correct. It is UB, but we cannot avoid it.
		const auto result
		{
			libusb_interrupt_transfer(device, endpoint,
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
				const_cast<uint8_t *>(static_cast<const uint8_t *>(bufferPtr)), bufferLen, nullptr, 0)
		};

		if (result)
		{
			const auto endpointNumber{uint8_t(endpoint & 0x7FU)};
			const auto direction{endpointDir_t(endpoint & 0x80U)};
			console.error("Failed to complete interrupt transfer of "sv, bufferLen,
				" byte(s) to endpoint "sv, endpointNumber, ' ',
				direction == endpointDir_t::controllerIn ? "IN"sv : "OUT"sv,
				", reason:"sv, libusb_error_name(result));
		}
		return !result;
	}

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] bool bulkTransfer(const uint8_t endpoint, const void *const bufferPtr,
		const int32_t bufferLen) const noexcept
	{
		// The const-cast here is required becasue libusb is not const-correct. It is UB, but we cannot avoid it.
		const auto result
		{
			libusb_bulk_transfer(device, endpoint,
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
				const_cast<uint8_t *>(static_cast<const uint8_t *>(bufferPtr)), bufferLen, nullptr, 0)
		};
		if (result)
		{
			const auto endpointNumber{uint8_t(endpoint & 0x7FU)};
			const auto direction{endpointDir_t(endpoint & 0x80U)};
			console.error("Failed to complete bulk transfer of "sv, bufferLen,
				" byte(s) to endpoint "sv, endpointNumber, ' ',
				direction == endpointDir_t::controllerIn ? "IN"sv : "OUT"sv,
				", reason:"sv, libusb_error_name(result));
		}
		return !result;
	}

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] bool controlTransfer(const requestType_t requestType, const uint8_t request, const uint16_t value,
		const uint16_t index, const void *const bufferPtr, const uint16_t bufferLen) const noexcept
	{
		// The const-cast here is required becasue libusb is not const-correct. It is UB, but we cannot avoid it.
		const auto result
		{
			libusb_control_transfer(device, requestType, request, value, index,
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
				const_cast<uint8_t *>(static_cast<const uint8_t *>(bufferPtr)), bufferLen, 0)
		};
		if (result < 0)
		{
			console.error("Failed to complete control transfer of "sv, bufferLen,
				" bytes(s), reason:"sv, libusb_error_name(result));
		}
		else if (result != bufferLen)
		{
			console.error("Control transfer incomplete, got "sv, result,
				", expected "sv, bufferLen);
		}
		return result == bufferLen;
	}

public:
	usbDeviceHandle_t() noexcept = default;
	usbDeviceHandle_t(libusb_device_handle *const device_) noexcept : device{device_}
		{ autoDetachKernelDriver(true); }
	~usbDeviceHandle_t() noexcept
		{ libusb_close(device); }
	[[nodiscard]] bool valid() const noexcept { return device; }

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	void autoDetachKernelDriver(bool autoDetach) const noexcept
	{
		if (const auto result{libusb_set_auto_detach_kernel_driver(device, autoDetach)}; result)
			console.warning("Automatic detach of kernel driver not supported on this platform"sv);
	}

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] bool claimInterface(const int32_t interfaceNumber) const noexcept
	{
		const auto result{libusb_claim_interface(device, interfaceNumber)};
		if (result)
			console.error("Failed to claim interface "sv, interfaceNumber, ": "sv, libusb_error_name(result));
		return !result;
	}

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] bool releaseInterface(const int32_t interfaceNumber) const noexcept
	{
		const auto result{libusb_release_interface(device, interfaceNumber)};
		if (result)
			console.error("Failed to release interface "sv, interfaceNumber, ": "sv, libusb_error_name(result));
		return !result;
	}

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] std::string readStringDescriptor(const uint8_t stringIndex,
		const uint16_t languageID = 0x0409U) const noexcept
	{
		// Guard on trying to read string index 0 as doing so is USB UB
		if (!stringIndex)
			return {};
		// Set up a 512 byte array to read the string into, and try to read it
		std::array<uint8_t, 512> stringData{};
		const auto result
		{
			libusb_get_string_descriptor(device, stringIndex, languageID, stringData.data(), stringData.size())
		};
		// Check for USB errors and report them
		if (result < 0)
		{
			console.error("Failed to read string descriptor "sv, stringIndex, " for language ",
				asHex_t<4, '0'>(languageID), ", reason:"sv, libusb_error_name(result));
			return {};
		}
		assert(result >= 2);
		// The result is the complete descriptor, header and all, so discard the header bytes
		// and subtract that from the length to decode.
		const auto stringLength{static_cast<size_t>(result - 2) / 2U};
		// Set up a string of appropriate length to receive the string data into
		std::u16string string(stringLength + 1U, u'\0');
		// Copy the UTF-16 string data into the new string, convert it to UTF-8 and return it
		std::memcpy(string.data(), stringData.data() + 2U, stringLength * 2U);
		return utf16::convert(string);
	}

	[[nodiscard]] bool writeInterrupt(const uint8_t endpoint, const void *const bufferPtr, const int32_t bufferLen) const noexcept
		{ return interruptTransfer(endpointAddress(endpointDir_t::controllerOut, endpoint), bufferPtr, bufferLen); }

	[[nodiscard]] bool readInterrupt(const uint8_t endpoint, void *const bufferPtr, const int32_t bufferLen) const noexcept
		{ return interruptTransfer(endpointAddress(endpointDir_t::controllerIn, endpoint), bufferPtr, bufferLen); }

	[[nodiscard]] bool writeBulk(const uint8_t endpoint, const void *const bufferPtr, const int32_t bufferLen) const noexcept
		{ return bulkTransfer(endpointAddress(endpointDir_t::controllerOut, endpoint), bufferPtr, bufferLen); }

	[[nodiscard]] bool readBulk(const uint8_t endpoint, void *const bufferPtr, const int32_t bufferLen) const noexcept
		{ return bulkTransfer(endpointAddress(endpointDir_t::controllerIn, endpoint), bufferPtr, bufferLen); }

	template<typename T> bool writeControl(requestType_t requestType, const uint8_t request,
		const uint16_t value, const uint16_t index, const T &data) const noexcept
	{
		requestType.dir(endpointDir_t::controllerOut);
		static_assert(sizeof(T) <= UINT16_MAX);
		return controlTransfer(requestType, request, value, index, &data, sizeof(T));
	}

	[[nodiscard]] bool writeControl(requestType_t requestType, const uint8_t request,
		const uint16_t value, const uint16_t index, std::nullptr_t) const noexcept
	{
		requestType.dir(endpointDir_t::controllerOut);
		return controlTransfer(requestType, request, value, index, nullptr, 0);
	}

	template<typename T> bool readControl(requestType_t requestType, const uint8_t request,
		const uint16_t value, const uint16_t index, T &data) const noexcept
	{
		requestType.dir(endpointDir_t::controllerIn);
		static_assert(sizeof(T) <= UINT16_MAX);
		return controlTransfer(requestType, request, value, index, &data, sizeof(T));
	}

	template<typename T, size_t length> bool readControl(requestType_t requestType, const uint8_t request,
		const uint16_t value, const uint16_t index, std::array<T, length> &data) const noexcept
	{
		requestType.dir(endpointDir_t::controllerIn);
		static_assert(length * sizeof(T) <= UINT16_MAX);
		return controlTransfer(requestType, request, value, index, data.data(), length * sizeof(T));
	}
};

struct usbDevice_t final
{
private:
	libusb_device *device{nullptr};
	libusb_device_descriptor descriptor{};

	usbDevice_t() noexcept = default;

public:
	usbDevice_t(libusb_device *const device_) noexcept : device{device_}
	{
		libusb_ref_device(device);
		if (const auto result{libusb_get_device_descriptor(device, &descriptor)}; result)
		{
			console.warning("Failed to get descriptor for device: ", libusb_error_name(result));
			descriptor = {};
		}
	}

	usbDevice_t(const usbDevice_t &device_) noexcept : device{device_.device}, descriptor{device_.descriptor}
		{ libusb_ref_device(device); }

	usbDevice_t(usbDevice_t &&other) noexcept : usbDevice_t{} { swap(other); }
	usbDevice_t &operator =(const usbDevice_t &) noexcept = delete;

	// NOLINTNEXTLINE(modernize-use-equals-default)
	~usbDevice_t() noexcept
	{
		if (device)
			libusb_unref_device(device);
	}

	usbDevice_t &operator =(usbDevice_t &&other) noexcept
	{
		swap(other);
		return *this;
	}

	[[nodiscard]] auto vid() const noexcept { return descriptor.idVendor; }
	[[nodiscard]] auto pid() const noexcept { return descriptor.idProduct; }
	[[nodiscard]] auto manufacturerIndex() const noexcept { return descriptor.iManufacturer; }
	[[nodiscard]] auto productIndex() const noexcept { return descriptor.iProduct; }
	[[nodiscard]] auto serialNumberIndex() const noexcept { return descriptor.iSerialNumber; }

	[[nodiscard]] auto busNumber() const noexcept { return libusb_get_bus_number(device); }
	[[nodiscard]] auto portNumber() const noexcept { return libusb_get_port_number(device); }

	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] usbDeviceHandle_t open() const noexcept
	{
		libusb_device_handle *handle{nullptr};
		if (const auto result{libusb_open(device, &handle)}; result)
		{
			console.error("Failed to open requested device: "sv, libusb_error_name(result));
			return {};
		}
		return {handle};
	}

	[[nodiscard]] usbConfiguration_t activeConfiguration() const noexcept
	{
		libusb_config_descriptor *config = nullptr;
		if (const auto result{libusb_get_active_config_descriptor(device, &config)}; result)
		{
			console.error("Failed to get active configuration descriptor: "sv, libusb_error_name(result));
			return {};
		}
		return {config};
	}

	void swap(usbDevice_t &other) noexcept
	{
		std::swap(device, other.device);
		std::swap(descriptor, other.descriptor);
	}
};

#endif /*USB_DEVICE_HXX*/
