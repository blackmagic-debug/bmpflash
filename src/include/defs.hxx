// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
// SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
#ifndef DEFS_HXX
#define DEFS_HXX

#if defined(__has_include) && __has_include(<version>)
#include <version>
#endif

#if (defined(__has_cpp_attribute) && __has_cpp_attribute(gnu::packed)) || defined(__GNUC__)
#define BEGIN_PACKED(n)
#define END_PACKED()
#define ATTR_PACKED [[gnu::packed]]
#elif defined(_MSC_VER)
#define STRINGIFY(n) #n
#define BEGIN_PACKED(n) __pragma(pack(push, n))
#define END_PACKED() __pragma(pack(pop))
#define ATTR_PACKED
#endif

#endif /*DEFS_HXX*/
