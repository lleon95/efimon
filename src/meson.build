#
# See LICENSE for more information about licensing
#  Copyright 2023
#
# Author: Luis G. Leon Vega <luis.leon@ieee.org>
#

lib_efimon_sources = []

# -----------------------------------------------------------------------------
# Library compilation
# -----------------------------------------------------------------------------

subdir('efimon')
libefimon = shared_library('efimon' ,
  lib_efimon_sources, 
  cpp_args : cpp_args,
  install : true,
  include_directories : [project_inc],
  dependencies : [project_deps]
)

libefimon_dep = declare_dependency(link_with: libefimon)

# -----------------------------------------------------------------------------
# Tools compilation
# -----------------------------------------------------------------------------

subdir('tools')
