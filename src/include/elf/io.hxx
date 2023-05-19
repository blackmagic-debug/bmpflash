// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 The Mangrove Language
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef ELF_IO_HXX
#define ELF_IO_HXX

#include <cstdint>
#include <array>
#include <cstring>
#include <type_traits>
#include <substrate/span>
#include "enums.hxx"

/**
 * @file io.hxx
 * @brief Types and functions for performing IO on ELF files
 */

namespace bmpflash::elf::io
{
	using substrate::span;

	inline namespace internal
	{
		using bmpflash::elf::enums::endian_t;

		/**
		 * A light-weight IO container that understands how to read and write
		 * to a span in an endian-aware manner
		 */
		struct container_t final
		{
		private:
			span<uint8_t> _data;

		public:
			container_t(const span<uint8_t> &data) : _data{data} { }

			void fromBytes(void *const value) const noexcept
				{ std::memcpy(value, _data.data(), _data.size()); }

			void fromBytesLE(uint16_t &value) const noexcept
			{
				std::array<uint8_t, 2> data{};
				fromBytes(data.data());
				value = uint16_t((uint16_t(data[1]) << 8U) | data[0]);
			}

			void fromBytesLE(uint32_t &value) const noexcept
			{
				std::array<uint8_t, 4> data{};
				fromBytes(data.data());
				value =
					(uint32_t(data[3]) << 24U) | (uint32_t(data[2]) << 16U) |
					(uint32_t(data[1]) << 8U) | data[0];
			}

			void fromBytesLE(uint64_t &value) const noexcept
			{
				std::array<uint8_t, 8> data{};
				fromBytes(data.data());
				value =
					(uint64_t(data[7]) << 56U) | (uint64_t(data[6]) << 48U) |
					(uint64_t(data[5]) << 40U) | (uint64_t(data[4]) << 32U) |
					(uint64_t(data[3]) << 24U) | (uint64_t(data[2]) << 16U) |
					(uint64_t(data[1]) << 8U) | data[0];
			}

			template<typename T> std::enable_if_t<
				std::is_integral_v<T> && !std::is_same_v<T, bool> &&
				std::is_signed_v<T> && sizeof(T) >= 2
			> fromBytesLE(T &value) const noexcept
			{
				std::make_unsigned_t<T> data{};
				fromBytesLE(data);
				value = static_cast<T>(data);
			}

			template<typename T> std::enable_if_t<std::is_enum_v<T>> fromBytesLE(T &value) const noexcept
			{
				std::underlying_type_t<T> data{};
				fromBytesLE(data);
				value = static_cast<T>(data);
			}

			void fromBytesBE(uint16_t &value) const noexcept
			{
				std::array<uint8_t, 2> data{};
				fromBytes(data.data());
				value = uint16_t((data[0] << 8U) | data[1]);
			}

			void fromBytesBE(uint32_t &value) const noexcept
			{
				std::array<uint8_t, 4> data{};
				fromBytes(data.data());
				value =
					(uint32_t(data[0]) << 24U) | (uint32_t(data[1]) << 16U) |
					(uint32_t(data[2]) << 8U) | data[3];
			}

			void fromBytesBE(uint64_t &value) const noexcept
			{
				std::array<uint8_t, 8> data{};
				fromBytes(data.data());
				value =
					(uint64_t(data[0]) << 56U) | (uint64_t(data[1]) << 48U) |
					(uint64_t(data[2]) << 40U) | (uint64_t(data[3]) << 32U) |
					(uint64_t(data[4]) << 24U) | (uint64_t(data[5]) << 16U) |
					(uint64_t(data[6]) << 8U) | data[7];
			}

			template<typename T> std::enable_if_t<
				std::is_integral_v<T> && !std::is_same_v<T, bool> &&
				std::is_signed_v<T> && sizeof(T) >= 2
			> fromBytesBE(T &value) const noexcept
			{
				std::make_unsigned_t<T> data{};
				fromBytesBE(data);
				value = static_cast<T>(data);
			}

			template<typename T> std::enable_if_t<std::is_enum_v<T>> fromBytesBE(T &value) const noexcept
			{
				std::underlying_type_t<T> data{};
				fromBytesBE(data);
				value = static_cast<T>(data);
			}
		};

		/**
		 * ELF reading orchestration type - this deals with creating
		 * a suitable small subspan from the input span, and provides
		 * value-generating read functions to retreive typed data
		 * from the span.
		 * This includes doing endian dispatch to retrieve the data.
		 */
		template<typename T> struct reader_t
		{
		private:
			container_t _data;

		public:
			reader_t(const span<uint8_t> &data) noexcept : _data{data.subspan(0, sizeof(T))} { }

			[[nodiscard]] auto read() const noexcept
			{
				T value{};
				_data.fromBytes(&value);
				return value;
			}

			[[nodiscard]] auto read(const endian_t endian) const noexcept
			{
				if (endian == endian_t::little)
					return readLE();
				return readBE();
			}

			[[nodiscard]] auto readLE() const noexcept
			{
				T value{};
				_data.fromBytesLE(value);
				return value;
			}

			[[nodiscard]] auto readBE() const noexcept
			{
				T value{};
				_data.fromBytesBE(value);
				return value;
			}
		};

		/** This works similarly to the base reader type, but on std::array<>'s */
		template<typename T, size_t N> struct reader_t<std::array<T, N>>
		{
		private:
			container_t _data;

		public:
			reader_t(const span<uint8_t> &data) noexcept : _data{data.subspan(0, sizeof(T) * N)} { }

			[[nodiscard]] auto read() const noexcept
			{
				std::array<T, N> value{};
				_data.fromBytes(value.data());
				return value;
			}
		};
	} // namespace internal

	/**
	 * Representation of a block of memory that can be read or written to in
	 * an endian-aware manner, allowing interaction with the blocks of an ELF file
	 */
	struct memory_t final
	{
	private:
		span<uint8_t> _data;

	public:
		constexpr memory_t(span<uint8_t> storage) noexcept : _data{storage} { }

		[[nodiscard]] constexpr const auto &dataSpan() const noexcept { return _data; }
		[[nodiscard]] constexpr const auto *data() const noexcept { return _data.data(); }
		[[nodiscard]] constexpr auto length() const noexcept { return _data.size(); }

		template<typename T> [[nodiscard]] auto read(const size_t offset) const noexcept
			{ return reader_t<T>{_data.subspan(offset)}.read(); }

		template<typename T> [[nodiscard]] auto read(const size_t offset, const endian_t endian) const noexcept
			{ return reader_t<T>{_data.subspan(offset)}.read(endian); }
	};

	/** Helper type for std::visit(), allowing match block semantics for interaction with std::variant<>s */
	template<typename... Ts> struct match_t : Ts... { using Ts::operator()...; };
	template<typename... Ts> match_t(Ts...) -> match_t<Ts...>;
} // namespace bmpflash:::elf::io

#endif /*ELF_IO_HXX*/
