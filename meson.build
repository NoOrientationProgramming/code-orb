
project(
	'CodeOrb - Microcontroller Debugging',
	'cpp',
)

cxx = meson.get_compiler('cpp')

# Sources

srcs = [
	'ProcessingCore/Processing.cpp',
	'ProcessingCore/Log.cpp',
	'ProcessingCore/SystemDebugging.cpp',
	'ProcessingCore/SystemCommanding.cpp',
	'ProcessingCore/TcpListening.cpp',
	'ProcessingCore/TcpTransfering.cpp',
	'ProcessingCommon/Res.cpp',
	'ProcessingCommon/LibTime.cpp',
	'ProcessingCommon/LibDspc.cpp',
	'ProcessingCommon/KeyUser.cpp',
	'src/main.cpp',
	'src/GwSupervising.cpp',
	'src/GwMsgDispatching.cpp',
	'src/SingleWireControlling.cpp',
	'src/RemoteCommanding.cpp',
	'src/LibUart.cpp',
	'src/TelnetFiltering.cpp',
]

# Arguments

args = [
	'-DdPackageName="' + meson.project_name() + '"',
	'-DdAppName="codeorb"',
	'-DCONFIG_PROC_HAVE_LOG=1',
	'-DCONFIG_CMD_SIZE_BUFFER_OUT=2048',
]

# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
warningsLinux = [
	'-Wall',
	'-Wextra',
	'-Wpedantic',
	'-Werror',
	'-Wfatal-errors',
	'-Wreorder',
	'-Wswitch-enum',
	'-Wuseless-cast',
	'-Wparentheses',
	'-Wshift-overflow',
	'-Wsign-compare',
	'-Wzero-as-null-pointer-constant',
	'-Wcast-align',
	'-Wcast-qual',
	'-Wcatch-value',
	'-Wchar-subscripts',
	'-Wswitch-default',
	'-Wctor-dtor-privacy',
	'-Wduplicated-branches',
	'-Wduplicated-cond',
	'-Wempty-body',
	'-Wextra-semi',
	'-Wfloat-equal',
	'-Wformat',
	'-Wformat-extra-args',
	'-Wimplicit-fallthrough',
	'-Wmissing-field-initializers',
	'-Wnull-dereference',
	'-Wshadow',
]

if host_machine.system() == 'windows'
	args += '-D_WIN32_WINNT=_WIN32_WINNT_WIN10'
	args += '-DWINVER=_WIN32_WINNT_WIN10'
else
	args += '-std=gnu++11'
endif

if cxx.get_id() == 'msvc'
	args += '/std:c11'
elif cxx.get_id() == 'gcc'
	args += warningsLinux
endif

# Dependencies

deps = []
deps += dependency('threads')
deps += dependency('tclap')

if host_machine.system() == 'windows'
	deps += cxx.find_library('ws2_32')
endif

# Application

myApp = executable(
	'codeorb',
	[
		srcs,
	],
	include_directories : include_directories([
		'./ProcessingCore',
		'./ProcessingCommon',
		'./src',
	]),
	dependencies : [
		deps,
	],
	cpp_args : [
		args,
	],
)

