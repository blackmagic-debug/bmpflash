# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2023 1BitSquared <info@1bitsquared.com>
# SPDX-FileContributor: Written by Rachel Mant <git@dragonmux.network>
project(
	'bmpflash',
	'cpp',
	default_options: [
		'cpp_std=c++17',
		'warning_level=3',
		'buildtype=release',
		'strip=true',
		'b_debug=if-release',
		'b_lto=true'
	],
	version: '0.0.1',
	meson_version: '>= 0.58',
	subproject_dir: 'deps'
)

sanitizer = get_option('b_sanitize')
coverage = get_option('b_coverage')
noUndefined = get_option('b_lundef')
debug = get_option('debug')
isWindows = host_machine.system() == 'windows'

subdir('src')
