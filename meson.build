#
# See LICENSE for more information about licensing
#  Copyright 2023
#
# Author: Luis G. Leon Vega <luis.leon@ieee.org>
#

project('efimon', ['c','cpp'], version : '0.1.0',
        default_options : ['warning_level=3',
                           'cpp_std=c++17',
                           'c_std=c17',
                           ],
        license: 'LGPL-2.1-only',
        meson_version: '>= 1.0.0' 
       )

# -----------------------------------------------------------------------------
# Global variables and default definitions
# -----------------------------------------------------------------------------
c_args = ['-Werror']
cpp_args = ['-Werror']
cmake = import('cmake')
fs = import('fs')

project_inc = [include_directories('include')]
project_deps = []

# Find proc library
cpp = meson.get_compiler('cpp')
libprocps_dep = cpp.find_library('libprocps', required: true)
project_deps += libprocps_dep

# Find SQLite3
sqlite_dep = dependency('sqlite3', required: false)
if sqlite_dep.found()
  project_deps += sqlite_dep
endif

# Verify if Perf is installed
perf_executable = find_program('perf', required: false, native: true)
if perf_executable.found()
  message('Linux Perf found. Enabling')
  cpp_args += ['-DENABLE_PERF']
else
  warning('Linux Perf not found. Disabling')
endif

# Verify if IPMI OEM is installed
ipmi_oem_executable = find_program('ipmi-oem', required: false, native: true)
if ipmi_oem_executable.found()
  message('IPML Found. Enabling')
  cpp_args += ['-DENABLE_IPMI']
else
  warning('IPMI not found. Disabling')
endif

# Verify if RAPL exists
if fs.is_dir('/sys/class/powercap/intel-rapl')
  message('RAPL Found. Enabling')
  cpp_args += ['-DENABLE_RAPL']
else
  warning('RAPL not found. Disabling')
endif

# Add Intel PCM
if get_option('enable-pcm')
  # Configure the CMake project
  opt_var = cmake.subproject_options()
  opt_var.set_override_option('warning_level', '0')
  pcm_proj = cmake.subproject('pcm', options: opt_var)

  # Fetch the dependency object
  pcm_dep = pcm_proj.dependency('PCM_SHARED')
  project_deps += pcm_dep
endif

# -----------------------------------------------------------------------------
# Developer mode options
# -----------------------------------------------------------------------------

if get_option('developer-mode')
  pre_commit = find_program('pre-commit', required: true, native: true)
  run_command(pre_commit, 'install', check: false)
  run_command(pre_commit, 'install', '--hook-type', 'commit-msg', check: false)
  c_args += ['-DDEBUG_MODE']
  cpp_args += ['-DDEBUG_MODE']
endif

# -----------------------------------------------------------------------------
# Subdirectories
# -----------------------------------------------------------------------------

if not get_option('build-docs-only')
  subdir('include/efimon')
  subdir('src')

  # Imports pkgconfig module
  pkgconfig = import('pkgconfig')
  pkgconfig_install_dir = join_paths(get_option('libdir'), 'pkgconfig')
  pkgconfig.generate(libefimon,
                     description: 'Implementation-Agnostic Efficiency Monitor')
endif

if get_option('build-examples')
  subdir('examples')
endif

if get_option('build-docs') or get_option('build-docs-only')
  subdir('docs-src')
endif
