// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 The Mangrove Language
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef FLAGS_HXX
#define FLAGS_HXX

#include <cstdint>
#include <type_traits>

namespace bmpflash
{
	template<typename T, typename enum_t> struct bitFlags_t
	{
	private:
		static_assert(std::is_integral_v<T>, "bitFlags_t must be backed by an integral type");
		static_assert(std::is_enum_v<enum_t>, "bitFlags_t must be enumerated by an enum");
		T value{};

		using enumInt_t = std::underlying_type_t<enum_t>;
		constexpr static T flagAsBit(const enum_t flag) noexcept { return T(UINT64_C(1) << enumInt_t(flag)); }

		// Internal value constructor to make .without() work
		constexpr bitFlags_t(const T &flags) noexcept : value{flags} { }

	public:
		constexpr bitFlags_t() noexcept = default;
		constexpr bitFlags_t(const bitFlags_t &flags) noexcept : value{flags.value} { }
		// move ctor omitted as it doesn't really make sense for this type
		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			constexpr bitFlags_t(const values_t ...flags) noexcept : value{(flagAsBit(flags) | ...)} { }

		constexpr bitFlags_t &operator =(const bitFlags_t &flags) noexcept
		{
			if (&flags != this)
				value = flags.value;
			return *this;
		}

		// Move assignment omitted as it doesn't really make sense for this type.

		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			constexpr bitFlags_t &operator =(const values_t ...flags) noexcept
		{
			value = (flagAsBit(flags) | ...);
			return *this;
		}

		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			constexpr void set(const values_t ...flags) noexcept { value |= (flagAsBit(flags) | ...); }

		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			constexpr void clear(const values_t ...flags) noexcept { value &= T(~(flagAsBit(flags) | ...)); }

		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			[[nodiscard]] constexpr bool includes(const values_t ...flags) const noexcept
		{
			const T bits{(flagAsBit(flags) | ...)};
			return value & bits;
		}

		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			[[nodiscard]] constexpr bitFlags_t without(const values_t ...flags) const noexcept
		{
			const auto bits{(flagAsBit(flags) | ...)};
			const auto newValue{value & ~bits};
			return {T(newValue)};
		}

		[[nodiscard]] constexpr auto toRaw() const noexcept { return value; }

		constexpr bool operator ==(const bitFlags_t &flags) const noexcept { return value == flags.value; }
		constexpr bool operator ==(const enum_t flag) const noexcept { return value == flagAsBit(flag); }
		constexpr bool operator !=(const bitFlags_t &flags) const noexcept { return value != flags.value; }
		constexpr bool operator !=(const enum_t flag) const noexcept { return value != flagAsBit(flag); }
	};

	template<typename enum_t> struct flags_t
	{
	private:
		static_assert(std::is_enum_v<enum_t>, "flags_t must be enumerated by an enum");
		using enumInt_t = std::underlying_type_t<enum_t>;

		enumInt_t value{};

	public:
		constexpr flags_t() noexcept = default;
		constexpr flags_t(const flags_t &flags) noexcept : value{flags.value} { }
		// move ctor omitted as it doesn't really make sense for this type
		constexpr flags_t(const enumInt_t &flags) noexcept : value{flags} { }
		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			constexpr flags_t(const values_t ...flags) noexcept : value{(enumInt_t(flags) | ...)} { }

		constexpr flags_t &operator =(const flags_t &flags) noexcept
		{
			if (&flags != this)
				value = flags.value;
			return *this;
		}

		// Move assignment omitted as it doesn't really make sense for this type.

		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			constexpr flags_t &operator =(const values_t ...flags) noexcept
		{
			value = (enumInt_t(flags) | ...);
			return *this;
		}

		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			constexpr void set(const values_t ...flags) noexcept { value |= (enumInt_t(flags) | ...); }

		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			constexpr void clear(const values_t ...flags) noexcept { value &= enumInt_t(~(enumInt_t(flags) | ...)); }

		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			[[nodiscard]] constexpr bool includes(const values_t ...flags) const noexcept
		{
			const auto bits{(enumInt_t(flags) | ...)};
			return value & bits;
		}

		template<typename... values_t, typename = std::enable_if_t<(std::is_same_v<values_t, enum_t> && ...)>>
			[[nodiscard]] constexpr flags_t without(const values_t ...flags) const noexcept
		{
			const auto bits{(enumInt_t(flags) | ...)};
			const auto newValue{value & ~bits};
			return {enumInt_t(newValue)};
		}

		[[nodiscard]] constexpr auto toRaw() const noexcept { return value; }

		constexpr bool operator ==(const flags_t &flags) const noexcept { return value == flags.value; }
		constexpr bool operator ==(const enum_t flag) const noexcept { return value == flagAsBit(flag); }
		constexpr bool operator !=(const flags_t &flags) const noexcept { return value != flags.value; }
		constexpr bool operator !=(const enum_t flag) const noexcept { return value != flagAsBit(flag); }
	};
} // namespace bmpflash

#endif /*FLAGS_HXX*/
