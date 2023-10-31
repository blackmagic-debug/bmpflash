// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <fmt/format.h>
#include <substrate/console>
#include "windows/serialInterface.hxx"
#include "bmp.hxx"

using substrate::console;

constexpr static auto uncDeviceSuffix{"\\\\.\\"sv};

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
	char *message{nullptr};
	// Ask Windows to please tell us what the error value we have means, and allocate + store it in `message`
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<char *>(&message), 0, nullptr);
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

	~hklmRegistryKey_t() noexcept
	{
		// Close the key if we're destructing a valid object that holds a handle to one
		if (handle != INVALID_HANDLE_VALUE)
			RegCloseKey(handle);
	}

	[[nodiscard]] bool valid() const noexcept { return handle != INVALID_HANDLE_VALUE; }

	// Reads the string key `keyName` from the registry
	[[nodiscard]] std::string readStringKey(std::string_view keyName) const
	{
		DWORD valueLength{0U};
		// Figure out how long the value is, stored into `valueLength`
		if (const auto result{RegGetValue(handle, nullptr, keyName.data(), RRF_RT_REG_SZ, nullptr, nullptr, &valueLength)};
			result != ERROR_SUCCESS && result != ERROR_MORE_DATA)
		{
			displayError(result, "retrieve value for key"sv, keyName);
			return {};
		}

		// Allocate a string long enough that has been prefilled with nul characters
		std::string value(valueLength, '\0');
		// Retrieve the actual value of the key now we have all the inforamtion needed to do so
		if (const auto result{RegGetValue(handle, nullptr, keyName.data(), RRF_RT_REG_SZ, nullptr, value.data(),
			&valueLength)}; result != ERROR_SUCCESS)
		{
			displayError(result, "retrieve value for key"sv, keyName);
			return {};
		}
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
				portName.c_str(),                               // UNC path to the target node
				GENERIC_READ | GENERIC_WRITE,                   // Standard file permissions
				0,                                              // With no sharing
				nullptr,                                        // Default security attributes
				OPEN_EXISTING,                                  // Only succeed if the node already exists
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, // Normal but unbuffered I/O
				nullptr                                         // No template file
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
	timeouts.ReadIntervalTimeout = 10;
	timeouts.ReadTotalTimeoutConstant = 10;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 10;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	if (!SetCommTimeouts(device, &timeouts))
		handleDeviceError("set communications timeouts for device"sv);

	// Having adjusted the line state, try to do a read of the serial state notification which will be sat in the buffer
	std::array<uint8_t, 10U> serialState{};
	static_cast<void>(ReadFile(device, serialState.data(), serialState.size(), nullptr, nullptr));
}

serialInterface_t::~serialInterface_t() noexcept
{
	if (device != INVALID_HANDLE_VALUE)
		CloseHandle(device);
}

void serialInterface_t::handleDeviceError(const std::string_view operation) noexcept
{
	// Get the last error that occured
	const auto error{GetLastError()};
	char *message{nullptr};
	// Ask Windows to please tell us what the error value we have means, and allocate + store it in `message`
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
		error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<char *>(&message), 0, nullptr);
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

std::string serialInterface_t::readPacket() const
{
	return "";
}