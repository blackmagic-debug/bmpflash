# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
# SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
cxx = meson.get_compiler('cpp')

extendedWarnings = [
	'-Wdouble-promotion',
	'-Wformat=2',
	'-Wformat-overflow=2',
	'-Wformat-signedness',
	'-Wformat-truncation',
	'-Wnull-dereference',
	'-Wmissing-attributes',
	'-Wmissing-braces',
	'-Wsequence-point',
	'-Wreturn-type',
	'-Wunused',
	'-Wunused-local-typedefs',
	'-Wunused-const-variable=2',
	'-Wmaybe-uninitialized',
	'-Wunknown-pragmas',
	'-Wstrict-aliasing',
	'-Wstrict-overflow=3',
	'-Wstring-compare',
	'-Wstringop-overflow',
	'-Warith-conversion',
	'-Wvla-parameter',
	'-Wduplicated-branches',
	'-Wshadow=local',
	'-Wunsafe-loop-optimizations',
	'-Wbad-function-cast',
	'-Wcast-qual',
	'-Wcast-align=strict',
	'-Wcast-function-type',
	'-Wconversion',
	'-Wdangling-else',
	'-Wsign-conversion',
	'-Wfloat-conversion',
#	'-Wpacked',
#	'-Wpadded',
	'-Wredundant-decls',
	'-Winline',
	'-Wvla',
	'-Wstack-protector',
	'-Wunsuffixed-float-constant',
	'-Wimplicit-fallthrough',
]

add_project_arguments(
	cxx.get_supported_arguments(extendedWarnings),
	language: 'cpp'
)

substrate = subproject(
	'substrate',
	default_options: [
		'build_tests=false',
		'default_library=static',
		'cpp_std=c++17',
	],
).get_variable(
	'substrate_dep'
)

libusb = dependency('libusb-1.0', version: '>=1.0.21', native: true)

bmpflashSrc = [
	'bmpflash.cxx'
]

bmpflash = executable(
	'bmpflash',
	bmpflashSrc,
	cpp_args: ['-D_FORTIFY_SOURCE=2'],
	include_directories: [include_directories('include')],
	dependencies: [substrate, libusb],
	gnu_symbol_visibility: 'inlineshidden'
)
