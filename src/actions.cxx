// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#include <string>
#include <string_view>
#include <substrate/indexed_iterator>
#include <substrate/console>
#include "actions.hxx"
#include "units.hxx"

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;
using substrate::indexedIterator_t;
using bmpflash::utils::humanReadableSize;

namespace bmpflash
{
	void displayInfo(size_t idx, const usbDevice_t &device);

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
			console.error("Failed to match devices based on serial number "sv, *deviceSerialNumber);
		}

		// Having checked for devices with a matching serial number, and failed, check if we've got just one device
		if (devices.size() == 1)
			return devices[0];

		// Otherwise, we're done here, error.
		console.error(devices.size(), " devices found, please use a serial number to select a specific one"sv);
		for (const auto [idx, device] : indexedIterator_t{devices})
			bmpflash::displayInfo(idx, device);
		return std::nullopt;
	}

	void displayInfo(const size_t idx, const usbDevice_t &device)
	{
		const auto handle{device.open()};
		// Read the 3 main string descriptors for the device
		const auto manufacturer{handle.readStringDescriptor(device.manufacturerIndex())};
		const auto product{handle.readStringDescriptor(device.productIndex())};
		const auto serialNumber
		{
			[&]()
			{
				auto value{handle.readStringDescriptor(device.serialNumberIndex())};
				if (value.empty())
					return "<no serial number>"s;
				return value;
			}()
		};

		console.info(idx + 1U, ": "sv, serialNumber, ", "sv, manufacturer, ", "sv, product);
	}

	int32_t displayInfo(const std::vector<usbDevice_t> &devices, const arguments_t &infoArguments)
	{
		// Check if the user's specified a specific serial number
		const auto *const serial{infoArguments["serial"sv]};
		if (serial)
		{
			// They did, so extract it and use it to filter the device list
			const auto &serialNumber{std::any_cast<std::string_view>(std::get<flag_t>(*serial).value())};
			const auto &device{filterDevices(devices, serialNumber)};
			if (!device)
				return 1;
			displayInfo(0, *device);
		}
		else
		{
			console.info(devices.size(), " devices found:"sv);
			// Loop through all the devices, displaying their information
			for (const auto [idx, device] : indexedIterator_t{devices})
				displayInfo(idx, device);
		}
		return 0;
	}
} // namespace bmpflash
