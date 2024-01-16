// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023a-2024 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <cstdint>
#include <cstddef>
#include <array>
#include <string>
#include <string_view>
#include <algorithm>
#include <memory>
#include <fmt/format.h>
#include <substrate/console>
#include "windows/serialInterface.hxx"
#include "usbDevice.hxx"
#include "bmp.hxx"

using namespace std::literals::string_view_literals;
using substrate::console;

constexpr static auto uncDeviceSuffix{"\\\\.\\"sv};

static std::array<char, 4096U> readBuffer{};
static size_t readBufferFullness{0U};
static size_t readBufferOffset{0U};

[[nodiscard]] std::string serialForDevice(const usbDevice_t &device)
{
	// Grab the serial number string descriptor index
	const auto serialIndex{device.serialNumberIndex()};
	// If it's invalid (0), return an invalid string
	if (serialIndex == 0)
		return {};
	// Otherwise open a handle on the device and try reading the string descriptor
	const auto handle{device.open()};
	return handle.readStringDescriptor(serialIndex);
}

void displayError(const LSTATUS error, const std::string_view operation, const std::string_view path)
{
	wchar_t *message{nullptr};
	// Ask Windows to please tell us what the error value we have means, and allocate + store it in `message`
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		static_cast<DWORD>(error), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<wchar_t *>(&message), 0,
		nullptr);
	console.error("Failed to "sv, operation, ' ', path, ", got error "sv, asHex_t<8, '0'>{uint32_t(error)},
		": "sv, message);
	// Clean up properly after ourselves
	LocalFree(message);
}

// This is a lightweight RAII wrapper around a HKEY on the HKLM registry hive.
// It could be generalised trivially to any kind of registry key, but it's not necessary for bmpflash.
struct hklmRegistryKey_t
{
private:
	HKEY handle{static_cast<HKEY>(INVALID_HANDLE_VALUE)};

public:
	hklmRegistryKey_t() noexcept = default;
	hklmRegistryKey_t(const hklmRegistryKey_t &) = delete;
	hklmRegistryKey_t operator =(const hklmRegistryKey_t &) = delete;

	// `path` should be a path somewhere inside the HKLM registry hive.
	hklmRegistryKey_t(const std::string &path, const REGSAM permissions) :
		handle
		{
			[&]()
			{
				auto keyHandle{static_cast<HKEY>(INVALID_HANDLE_VALUE)};
				// Try opening the requested key
				if (const auto result{RegOpenKeyEx(HKEY_LOCAL_MACHINE, path.c_str(), 0, permissions, &keyHandle)};
					result != ERROR_SUCCESS)
				{
					displayError(result, "open registry key"sv, path);
					return static_cast<HKEY>(INVALID_HANDLE_VALUE);
				}
				return keyHandle;
			}()
		} { }

	// NOLINTNEXTLINE(modernize-use-equals-default)
	~hklmRegistryKey_t() noexcept
	{
		// Close the key if we're destructing a valid object that holds a handle to one
		if (handle != INVALID_HANDLE_VALUE)
			RegCloseKey(handle);
	}

	[[nodiscard]] bool valid() const noexcept { return handle != INVALID_HANDLE_VALUE; }

	// Reads the string key `keyName` from the registry
	// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
	[[nodiscard]] std::string readStringKey(std::string_view keyName) const
	{
		DWORD valueLength{0U};
		// Figure out how long the value is, stored into `valueLength`
		if (const auto result{RegGetValueA(handle, nullptr, keyName.data(), RRF_RT_REG_SZ, nullptr, nullptr, &valueLength)};
			result != ERROR_SUCCESS && result != ERROR_MORE_DATA)
		{
			displayError(result, "retrieve value for key"sv, keyName);
			return {};
		}

		// Allocate a string long enough that has been prefilled with nul characters
		std::string value(valueLength, '\0');
		// Retrieve the actual value of the key now we have all the inforamtion needed to do so
		if (const auto result{RegGetValueA(handle, nullptr, keyName.data(), RRF_RT_REG_SZ, nullptr, value.data(),
			&valueLength)}; result != ERROR_SUCCESS)
		{
			displayError(result, "retrieve value for key"sv, keyName);
			return {};
		}

		// After, trim trailing nul characters as there will be 1 or 2
		while (!value.empty() && value.back() == '\0')
			value.erase(value.end() - 1U);
		return value;
	}
};

[[nodiscard]] std::string readKeyFromPath(const std::string &subpath, const std::string_view keyName)
{
	// Open the registry key that should represent the required subpath for the BMP
	const hklmRegistryKey_t keyPathHandle
	{
		fmt::format("SYSTEM\\CurrentControlSet\\Enum\\USB\\VID_{:04X}&PID_{:04X}{}"sv,
			bmp_t::vid, bmp_t::pid, subpath), KEY_READ
	};
	if (!keyPathHandle.valid())
		return {};
	// Retrieve the string key requested
	return keyPathHandle.readStringKey(keyName);
}

[[nodiscard]] std::string findBySerialNumber(const usbDevice_t &device)
{
	// Start by getting the serial number of the device
	const auto serialNumber{serialForDevice(device)};
	if (serialNumber.empty())
		return {};

	// Now look up the prefix for the entry in the interface 0s tree
	const auto prefix{readKeyFromPath(fmt::format("\\{}"sv, serialNumber), "ParentIdPrefix"sv)};
	if (prefix.empty())
		return {};
	console.debug("Device registry path prefix: "sv, prefix);

	// Look up the `COMn` device node name associated with the target interface
	auto portName{readKeyFromPath(fmt::format("&MI_00\\{}&0000\\Device Parameters"sv, prefix), "PortName"sv)};
	if (portName.empty())
		return {};

	// If it is not already a proper UNC device path, turn it into one
	if (std::string_view{portName}.substr(0, uncDeviceSuffix.size()) != uncDeviceSuffix)
		portName.insert(0, uncDeviceSuffix);

	console.info("Using "sv, portName, " for BMP remote protocol communications"sv);
	return portName;
}

serialInterface_t::serialInterface_t(const usbDevice_t &usbDevice) : device
	{
		[&]()
		{
			// Figure out what the device node is for the requested device
			const auto portName{findBySerialNumber(usbDevice)};
			if (portName.empty())
				return INVALID_HANDLE_VALUE;

			// Try and open the node so we can start communications with the the device
			return CreateFile
			(
				portName.c_str(),                                // UNC path to the target node
				GENERIC_READ | GENERIC_WRITE,                    // Standard file permissions
				0,                                               // With no sharing
				nullptr,                                         // Default security attributes
				OPEN_EXISTING,                                   // Only succeed if the node already exists
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, // Normal I/O w/ write-through
				nullptr                                          // No template file
			);
		}()
	}
{
	// If opening the device node failed for any reason, error out early
	if (device == INVALID_HANDLE_VALUE)
	{
		handleDeviceError("open device"sv);
		return;
	}

	// Get the current device state from the device
	DCB serialParams{};
	serialParams.DCBlength = sizeof(DCB);
	if (!GetCommState(device, &serialParams))
	{
		handleDeviceError("access communications state from device"sv);
		return;
	}

	// Adjust the device state to enable communications to work and be in the right mode
	serialParams.fParity = false;
	serialParams.fOutxCtsFlow = false;
	serialParams.fOutxDsrFlow = false;
	serialParams.fDtrControl = DTR_CONTROL_ENABLE;
	serialParams.fDsrSensitivity = false;
	serialParams.fOutX = false;
	serialParams.fInX = false;
	serialParams.fRtsControl = RTS_CONTROL_DISABLE;
	serialParams.ByteSize = 8;
	serialParams.Parity = NOPARITY;
	if (!SetCommState(device, &serialParams))
	{
		handleDeviceError("apply new communications state to device"sv);
		return;
	}

	COMMTIMEOUTS timeouts{};
	// Turn off read timeouts so that ReadFill() instantly returns even if there's no data waiting
	// (we implement our own mechanism below for that case as we only want to wait if we get no data)
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	// Configure an exactly 100ms write timeout - we want this triggering to be fatal as something
	// has gone very wrong if we ever hit this.
	timeouts.WriteTotalTimeoutConstant = 100;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	if (!SetCommTimeouts(device, &timeouts))
		handleDeviceError("set communications timeouts for device"sv);

	// Having adjusted the line state, discard anything sat in the receive buffer
	PurgeComm(device, PURGE_RXCLEAR);
}

// NOLINTNEXTLINE(modernize-use-equals-default)
serialInterface_t::~serialInterface_t() noexcept
{
	if (device != INVALID_HANDLE_VALUE)
		CloseHandle(device);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void serialInterface_t::handleDeviceError(const std::string_view operation) noexcept
{
	// Get the last error that occured
	const auto error{GetLastError()};
	// If there is no error and no device (we failed to look up the device node), return early
	if (error == ERROR_SUCCESS && device == INVALID_HANDLE_VALUE)
		return;
	wchar_t *message{nullptr};
	// Ask Windows to please tell us what the error value we have means, and allocate + store it in `message`
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<wchar_t *>(&message), 0, nullptr);
	console.error("Failed to "sv, operation, " ("sv, asHex_t<8, '0'>{error}, "): "sv, message);
	// Clean up properly after ourselves
	LocalFree(message);
	if (device != INVALID_HANDLE_VALUE)
		CloseHandle(device);
	device = INVALID_HANDLE_VALUE;
}

void serialInterface_t::swap(serialInterface_t &interface) noexcept
{
	std::swap(device, interface.device);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void serialInterface_t::writePacket(const std::string_view &packet) const
{
	console.debug("Remote write: "sv, packet);
	DWORD written{0};
	for (size_t offset{0}; offset < packet.length(); offset += written)
	{
		if (!WriteFile(device, packet.data() + offset, static_cast<DWORD>(packet.length() - offset), &written, nullptr))
		{
			console.error("Write to device failed ("sv, GetLastError(), "), written "sv, offset, " bytes");
			throw bmpCommsError_t{};
		}
	}
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void serialInterface_t::refillBuffer() const
{
	// Try to wait for up to 100ms for data to become available
	if (WaitForSingleObject(device, 100) != WAIT_OBJECT_0)
	{
		console.error("Waiting for data from device failed ("sv, GetLastError(), ")"sv);
		throw bmpCommsError_t{};
	}
	DWORD bytesReceived = 0;
	// Try to fill the read buffer, and if that fails, bail
	if (!ReadFile(device, readBuffer.data(), static_cast<DWORD>(readBuffer.size()), &bytesReceived, nullptr))
	{
		console.error("Read from device failed ("sv, GetLastError(), ")"sv);
		throw bmpCommsError_t{};
	}
	// We now have more data, so update the read buffer counters
	readBufferFullness = bytesReceived;
	readBufferOffset = 0U;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::string serialInterface_t::readPacket() const
{
	std::array<char, bmp_t::maxPacketSize> packet{};
	size_t length{0U};
	// Try gathering a '#' terminated response
	while (length < packet.size())
	{
		// Check if we need more data or should use what's in the buffer already
		if (readBufferOffset == readBufferFullness)
			refillBuffer();

		const auto *const bufferBegin{readBuffer.data() + readBufferOffset};
		const auto *const bufferEnd{readBuffer.data() + readBufferFullness};

		// Look for an end of message marker
		const auto *const eomMarker{std::find(bufferBegin, bufferEnd, static_cast<uint8_t>('#'))};
		// We now either have a remote end of message marker, or need all the data from the buffer
		std::uninitialized_copy(bufferBegin, eomMarker, packet.data() + length);
		const auto responseLength{static_cast<size_t>(std::distance(bufferBegin, eomMarker))};
		readBufferOffset += responseLength;
		length += responseLength;
		// If it's a remote end of message marker, break out the loop
		if (responseLength != readBufferFullness)
		{
			++readBufferOffset;
			break;
		}
	}

	// Make a new std::string of an appropriate length, copying the data in to return it
	// Skip the first byte to remove the beginning '&' (the ending '#' is already taken care of in the read loop)
	std::string result{packet.data() + 1U, length};
	console.debug("Remote read: "sv, result);
	return result;
}
