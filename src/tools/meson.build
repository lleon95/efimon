#
# See LICENSE for more information about licensing
#  Copyright 2023
#
# Author: Luis G. Leon Vega <luis.leon@ieee.org>
#

efimon_meter_sources = [
  files('efimon-meter.cpp')
]

executable('efimon-meter',
           efimon_meter_sources,
           cpp_args : cpp_args,
           include_directories : [project_inc],
           dependencies: [libefimon_dep],
           install : true,
)
