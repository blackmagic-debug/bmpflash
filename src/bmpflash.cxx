// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <vector>
#include <optional>
#include <string>
#include <substrate/indexed_iterator>
#include <substrate/console>
#include "usbContext.hxx"

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;
using substrate::indexedIterator_t;

[[nodiscard]] auto findBMPs(const usbContext_t &context)
{
	std::vector<usbDevice_t> devices{};
	for (auto device : context.deviceList())
	{
		if (device.vid() == 0x1d50U && device.pid() == 0x6018U)
		{
			console.info("Found BMP at USB address "sv, device.busNumber(), '-', device.portNumber());
			devices.emplace_back(std::move(device));
		}
	}
	return devices;
}

[[nodiscard]] std::optional<usbDevice_t> filterDevices(const std::vector<usbDevice_t> &devices,
	std::optional<std::string_view> deviceSerialNumber) noexcept
{
	if (deviceSerialNumber)
	{
		const auto targetSerialNumber{*deviceSerialNumber};
		for (const auto &device : devices)
		{
			const auto serialIndex{device.serialNumberIndex()};
			// If the device doesn't even have a serial number index, skip it for this phase.
			if (serialIndex == 0)
				continue;
			const auto handle{device.open()};
			const auto serialNumber{handle.readStringDescriptor(serialIndex)};
			if (serialNumber == targetSerialNumber)
				return device;
		}
	}

	// Having checked for devices with a matching serial number, and failed, check if we've got just one device
	if (devices.size() == 1)
		return devices[0];

	// Otherwise, we're done here, error.
	console.error(devices.size(), " devices found, please use a serial number to select a specific one"sv);
	for (const auto [idx, device] : indexedIterator_t{devices})
	{
		const auto handle{device.open()};
		// Read the 3 main string descriptors for the device
		const auto manufacturer{handle.readStringDescriptor(device.manufacturerIndex())};
		const auto product{handle.readStringDescriptor(device.productIndex())};
		const auto serialNumber
		{
			// This lambda takes device as a parameter because of our use of structured bindings
			// (they're not allowed to be captured)
			[&](const usbDevice_t &_device)
			{
				auto value{handle.readStringDescriptor(_device.serialNumberIndex())};
				if (value.empty())
					return "<no serial number>"s;
				return value;
			}(device)
		};

		console.info(idx + 1U, ": "sv, serialNumber, ", "sv, manufacturer, ", "sv, product);
	}
	return std::nullopt;
}

int main(int, char **)
{
	console = {stdout, stderr};

	const usbContext_t context{};
	if (!context.valid())
		return 2;

	const auto devices{findBMPs(context)};
	if (devices.empty())
	{
		console.error("Could not find any Black Magic Probes"sv);
		console.warn("Are you sure the permissions on the device are set correctly?"sv);
		return 1;
	}
	const auto device{filterDevices(devices, std::nullopt)};
	if (!device)
		return 1;

	return 0;
}
