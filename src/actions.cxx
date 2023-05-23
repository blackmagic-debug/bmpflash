// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#define SUBSTRATE_ALLOW_STD_FILESYSTEM_PATH

#include <string>
#include <string_view>
#include <substrate/span>
#include <substrate/fd>
#include <substrate/index_sequence>
#include <substrate/indexed_iterator>
#include <substrate/console>
#include <substrate/units>
#include "actions.hxx"
#include "flashVendors.hxx"
#include "sfdp.hxx"
#include "provisionELF.hxx"
#include "units.hxx"

using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;
using std::filesystem::path;
using substrate::indexSequence_t;
using substrate::indexedIterator_t;
using substrate::span;
using substrate::fd_t;
using substrate::normalMode;
using substrate::operator ""_KiB;
using bmpflash::utils::humanReadableSize;
using elfProvision_t = bmpflash::elf::provision_t;

namespace bmpflash
{
	void displayInfo(size_t idx, const usbDevice_t &device);

	[[nodiscard]] inline bool contains(const std::string_view &searchIn, const std::string_view &searchFor) noexcept
		{ return searchIn.find(searchFor) != std::string_view::npos; }

	std::optional<usbDevice_t> filterDevices(const std::vector<usbDevice_t> &devices,
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
				if (contains(serialNumber, targetSerialNumber))
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

	[[nodiscard]] spiDevice_t busToDevice(const spiBus_t &bus)
	{
		switch (bus)
		{
			case spiBus_t::internal:
				return spiDevice_t::intFlash;
			case spiBus_t::external:
				return spiDevice_t::extFlash;
			default:
				throw std::domain_error{"SPI bus requested is unhandled or unknown"};
		}
	}

	[[nodiscard]] std::optional<bmp_t> beginComms(const usbDevice_t &device, const spiBus_t &spiBus)
	{
		// Use the found device to then build the communications structure
		bmp_t probe{device};
		if (!probe.valid())
			return std::nullopt;

		// Initialise remote communications
		const auto probeVersion{probe.init()};
		console.info("Remote is "sv, probeVersion);

		// Convert the bus to use to a device too
		const auto spiDevice{busToDevice(spiBus)};

		// Start by checking the BMP is running a new enough remote protocol
		const auto protocolVersion{probe.readProtocolVersion()};
		if (protocolVersion < 3U || !probe.begin(spiBus, spiDevice))
		{
			console.error("Probe is running firmware that is too old, please update it");
			return std::nullopt;
		}
		return probe;
	}

	// This allows feeding a flag_t in for the bus instead of a raw spiBus_t
	[[nodiscard]] std::optional<bmp_t> beginComms(const usbDevice_t &device, const flag_t &bus)
		{ return beginComms(device, std::any_cast<spiBus_t>(bus.value())); }

	[[nodiscard]] std::string_view lookupFlashVendor(const uint8_t manufacturer) noexcept
	{
		// Look the Flash manufacturer up by MFR ID
		const auto vendor{flashVendors.find(manufacturer)};
		if (vendor == flashVendors.cend())
			return "<Unknown>"sv;
		return vendor->second;
	}

	bool identifyFlash(const bmp_t &probe) noexcept
	{
		const auto chipID{probe.identifyFlash()};
		// If we got a bad all-highs read back, or the capacity is 0, then there's no device there.
		if ((chipID.manufacturer == 0xffU && chipID.type == 0xffU && chipID.capacity == 0xffU) ||
			chipID.capacity == 0U)
		{
			console.error("Could not identify a valid Flash device on the requested SPI bus"sv);
			return false;
		}
		// Display some useful information about the Flash
		console.info("SPI Flash ID: ", asHex_t<2, '0'>{chipID.manufacturer}, ' ',
			asHex_t<2, '0'>{chipID.type}, ' ', asHex_t<2, '0'>{chipID.capacity});
		const auto flashSize{UINT32_C(1) << chipID.capacity};
		const auto [capacityValue, capacityUnits] = humanReadableSize(flashSize);
		console.info("Device is a "sv, capacityValue, capacityUnits, " device from "sv,
			lookupFlashVendor(chipID.manufacturer));
		return true;
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

	bool displaySFDP(const usbDevice_t &device, const arguments_t &sfdpArguments)
	{
		// Try to begin communications with the BMP
		auto probe{beginComms(device, std::get<flag_t>(*sfdpArguments["bus"sv]))};
		// If we got good comms, then try and identify the Flash
		if (!probe || !identifyFlash(*probe))
			return false;

		// Ask for the SFDP data and display it then clean up
		sfdp::readAndDisplay(*probe);
		return probe->end();
	}

	bool provision(const usbDevice_t &device, const arguments_t &provisionArguments)
	{
		// Try to begin communications with the BMP
		auto probe{beginComms(device, spiBus_t::internal)};
		// If we got good comms, then try and identify the Flash
		if (!probe || !identifyFlash(*probe))
			return false;

		// Try and open the requested file, checking that it's a valid ELF file
		const elfProvision_t elf{std::any_cast<path>(std::get<flag_t>(*provisionArguments["fileName"sv]).value())};
		if (!elf.valid())
		{
			console.error("Cannot read requested file as an ELF binary"sv);
			[[maybe_unused]] const auto result{probe->end()};
			return false;
		}
		// Now try and provision the requested binary to the on-board Flash
		console.info("Repacking ELF file for on-board SPI Flash and provisioning it to BMP"sv);
		if (!elf.repack(*probe))
		{
			console.error("Failed to successfully repack ELF file"sv);
			[[maybe_unused]] const auto result{probe->end()};
			return false;
		}

		// Finish up by cleaning up the session
		console.info("Repacking and provisioning complete"sv);
		return probe->end();
	}

	bool read(const usbDevice_t &device, const arguments_t &readArguments)
	{
		// Try to begin communications with the BMP
		auto probe{beginComms(device, std::get<flag_t>(*readArguments["bus"sv]))};
		// If we got good comms, then try and identify the Flash
		if (!probe || !identifyFlash(*probe))
			return false;

		auto spiFlash{sfdp::read(*probe)};
		if (!spiFlash)
		{
			console.error("Could not setup SPI Flash control structures"sv);
			[[maybe_unused]] const auto result{probe->end()};
			return false;
		}

		fd_t file{std::any_cast<path>(std::get<flag_t>(*readArguments["fileName"sv]).value()),
			O_WRONLY | O_CREAT | O_NOCTTY, normalMode};
		if (!file.valid())
		{
			console.error("Failed to open output file"sv);
			[[maybe_unused]] const auto result{probe->end()};
			return false;
		}

		const auto capacity{spiFlash->capacity()};
		std::array<uint8_t, 4_KiB> buffer{};
		for (const auto address : indexSequence_t{capacity}.step(buffer.size()))
		{
			const auto amount{std::min(capacity - address, buffer.size())};
			const span subspan{buffer.data(), amount};
			if (!spiFlash->readBlock(*probe, address, subspan))
			{
				console.error("SPI Flash readout failed"sv);
				[[maybe_unused]] const auto result{probe->end()};
				return false;
			}
			if (!file.write(subspan.data(), subspan.size()))
			{
				console.error("Failed to write data block to output file"sv);
				[[maybe_unused]] const auto result{probe->end()};
				return false;
			}
		}

		// Finish up by cleaning up the session
		console.info("SPI Flash chip read complete"sv);
		return probe->end();
	}
} // namespace bmpflash
