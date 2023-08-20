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
#	'-Winline',
	'-Wvla',
	'-Wstack-protector',
	'-Wunsuffixed-float-constant',
	'-Wimplicit-fallthrough',
	'-Wundef',
	'-Wignored-qualifiers',
	'-Wshadow',
	'-Wswitch-enum',
	'-Wdefaulted-function-deleted',
	'-Wdeprecated-copy',

	# MSVC unique warnings that are useful
	'-w35030', # attribute 'gnu::packed' is not recognized
	'-w35039', # 'identifier': pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc. Undefined behavior may occur if this function throws an exception.
	'-w35045', # Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified

	# -Wmaybe-uninitialized -- UB
	'-w34355', # 'this': used in base member initializer list

	# -Wconversion
	'-w34242', # 'initializing': conversion from '_Ty' to '_Ty1', possible loss of data
	'-w34244', # 'return': conversion from '_Ty' to '_Ty1', possible loss of data
	'-w34365', # 'identifier': conversion from 'identifier' to 'identifier', signed/unsigned mismatch
	'-w34800', # Implicit conversion from 'identifier' to bool. Possible information loss

	# -Wno-inline
	'-wd4710', # 'identifier': function not inlined
	'-wd4711', # function 'identifier' selected for automatic inline expansion

	# -Wunused
	'-w34514', # 'identifier': unreferenced inline function has been removed
	'-w35264', # 'identifier': 'const' variable is not used

	# -Wundef
	'-w34668', # '___cplusplus' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif''

	# -Wpacked -- disabling because MSVC uses pragmas for packing
	'-w34820', # 'identifier': 'X' bytes padding added after data member 'identifier'

	# -Wignored-qualifiers
	'-w35266', # 'const' qualifier on return type has no effect

	# -Wshadow
	'-w34458', # declaration of 'device' hides class member

	# -Wswitch-enum
	'-w34061', # enumerator 'identifier' in switch of enum 'identifier' is not explicitly handled by a case label

	# -Wdefaulted-function-deleted
	'-w34625', # 'identifier': copy constructor was implicitly defined as deleted
	'-w34626', # 'identifier': assignment operator was implicitly defined as deleted
	'-w34623', # 'identifier': default constructor was implicitly defined as deleted
	'-w35026', # 'identifier': move constructor was implicitly defined as deleted
	'-w35027', # 'identifier': move assignment operator was implicitly defined as deleted

	# -Wdeprecated(-copy)
	'-w35267', # definition of implicit copy constructor for 'identifier' is deprecated because it has a user-provided destructor'

	# Undocumented
	'-wd4582', # 'identifier': constructor is not implicitly called
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

fmt = subproject(
	'fmt',
	default_options: [
		'default_library=static',
	],
).get_variable(
	'fmt_dep'
)

libusb = dependency(
	'libusb-1.0',
	version: '>=1.0.21',
	method: 'pkg-config',
	fallback: 'libusb'
)

subdir('include')

bmpflashSrc = [
	'bmpflash.cxx', 'unicode.cxx', 'bmp.cxx', 'remoteSPI.cxx',
	'sfdp.cxx', 'actions.cxx', 'provisionELF.cxx', 'crc32.cxx',
	'spiFlash.cxx', versionHeader,
]

subdir('elf')

bmpflashCppArgs = ['-D_FORTIFY_SOURCE=2']
if host_machine.system() == 'windows'
	# libusb includes an unguarded windows.h header.
	bmpflashCppArgs += [
		'-DNOMINMAX',
		'-DWIN32_LEAN_AND_MEAN',
	]
endif

bmpflash = executable(
	'bmpflash',
	bmpflashSrc,
	cpp_args: bmpflashCppArgs,
	include_directories: [include_directories('include', 'include/elf')],
	dependencies: [substrate, fmt, libusb],
	gnu_symbol_visibility: 'inlineshidden'
)
