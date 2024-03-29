project('fmt', 'cpp', version: '10.2.0', license: 'BSD', default_options: ['cpp_std=c++14'])

fmt_private_cpp_args = []
fmt_interface_cpp_args = []
if get_option('default_library') == 'shared'
	fmt_private_cpp_args += ['-DFMT_LIB_EXPORT']
	fmt_interface_cpp_args += ['-DFMT_SHARED']
endif

fmt_lib = library(
	'fmt',
	'src/format.cc',
	'src/os.cc',
	cpp_args: fmt_private_cpp_args,
	include_directories: 'include',
)

fmt_dep = declare_dependency(
	include_directories: 'include',
	compile_args: fmt_interface_cpp_args,
	link_with: fmt_lib,
	variables: {
		'compile_args': '-I@0@'.format(meson.current_source_dir() / 'include'),
		'link_args': fmt_lib.full_path()
	},
)

if meson.version().version_compare('>=0.54.0')
	meson.override_dependency('fmt', fmt_dep)
endif

fmt_header_only_dep = declare_dependency(
	include_directories: 'include',
	compile_args: '-DFMT_HEADER_ONLY',
)
