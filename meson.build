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
                           'cpp_args=-Werror',
                           'c_args=-Werror'
                           ],
        license: 'LGPL-2.1-only',
        meson_version: '>= 1.0.0' 
       )

# -----------------------------------------------------------------------------
# Global variables and default definitions
# -----------------------------------------------------------------------------

project_inc = [include_directories('include')]
project_deps = []
c_args = []
cpp_args = []

# Find proc library
cpp = meson.get_compiler('cpp')
libprocps_dep = cpp.find_library('libprocps', required: true)
project_deps += libprocps_dep

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
  subdir('docs')
endif
