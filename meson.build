#
# See LICENSE for more information about licensing
#  Copyright 2023
#
# Author: Luis G. Leon Vega <luis.leon@ieee.org>
#

project('efimon', ['c','cpp'], version : '0.2.0',
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
c_args = ['-Werror']
cpp_args = ['-Werror']

# Find proc library
cpp = meson.get_compiler('cpp')
libprocps_dep = cpp.find_library('libprocps', required: true)
project_deps += libprocps_dep

# Find SQLite3
enable_sql = false
sqlite_dep = dependency('sqlite3', required: false)
if sqlite_dep.found() and get_option('enable-sql')
  message('SQLite Found. Enabling')
  enable_sql = true
  project_deps += sqlite_dep
  c_args += ['-DENABLE_SQLITE']
  cpp_args += ['-DENABLE_SQLITE']
else
  warning('SQLite dependency not found. Disabling loggers related to SQLite')
endif

# Verify if Perf is installed
enable_perf = false
perf_executable = find_program('perf', required: false, native: true)
if perf_executable.found() and get_option('enable-perf')
  enable_perf = true
  message('Linux Perf Found. Enabling')
  c_args += ['-DENABLE_PERF']
  cpp_args += ['-DENABLE_PERF']
else
  warning('Linux Perf not found. Disabling code related to Linux Perf')
endif

# Verify if IPMI OEM is installed
enable_ipmi = false
enable_ipmi_sensors = false
ipmi_oem_executable = find_program('ipmi-oem', required: false, native: true)
if ipmi_oem_executable.found() and get_option('enable-ipmi')
  enable_ipmi = true
  cpp_args += ['-DENABLE_IPMI']
  message('IPMI found')
else
  message('IPMI not found. Disabling code related to IPMI')
endif
ipmi_sensors_executable = find_program('ipmi-sensors', required: false, native: true)
if ipmi_sensors_executable.found() and get_option('enable-ipmi')
  enable_ipmi_sensors = true
  cpp_args += ['-DENABLE_IPMI_SENSORS']
  message('IPMI Sensors found')
else
  message('IPMI not found. Disabling code related to IPMI Sensors')
endif

# Add Intel PCM
enable_pcm = false
if get_option('enable-pcm')
  # Configure the CMake project
  opt_var = cmake.subproject_options()
  opt_var.set_override_option('warning_level', '0')
  pcm_proj = cmake.subproject('pcm', options: opt_var)

  # Fetch the dependency object
  pcm_dep = pcm_proj.dependency('PCM_SHARED')
  project_deps += pcm_dep
  enable_pcm = true
else
  warning('Intel PCM is disabled')
endif

# Verify if RAPL exists
enable_rapl = false
if fs.is_dir('/sys/class/powercap/intel-rapl') and get_option('enable-rapl')
  message('RAPL Found. Enabling')
  c_args += ['-DENABLE_RAPL']
  cpp_args += ['-DENABLE_RAPL']
  enable_rapl = true
else
  warning('RAPL not found. Disabling code related to Intel RAPL')
endif

# Verify if NVIDIA ML exists
enable_nvidia_dgpu = false
if get_option('enable-nvml')
  nvidia_ml_dep = cpp.find_library('nvidia-ml', required: false)
  cuda_dep = dependency('cuda', version : '>=10', required: false)
  if nvidia_ml_dep.found() and cuda_dep.found()
    project_deps += [nvidia_ml_dep, cuda_dep]
    c_args += ['-DENABLE_NVIDIA_NVML']
    cpp_args += ['-DENABLE_NVIDIA_NVML']
    enable_nvidia_dgpu = true
    message('NVIDIA NVML found')
  else
    warning('NVIDIA NVML not found')
  endif
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
