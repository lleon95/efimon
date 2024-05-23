#
# See LICENSE for more information about licensing
#  Copyright 2023-2024
#
# Author: Luis G. Leon Vega <luis.leon@ieee.org>
#

lib_efimon_sources += [
  files('arg-parser.cpp'),
  files('proc/io.cpp'),
  files('proc/list.cpp'),
  files('proc/meminfo.cpp'),
  files('proc/net.cpp'),
  files('proc/stat.cpp'),
  files('proc/thread-tree.cpp'),
  files('uptime.cpp'),
  files('asm-classifier.cpp'),
  files('asm-classifier/x86-classifier.cpp'),
  files('proc/cpuinfo.cpp'),
  files('process-manager.cpp'),
  files('logger/csv.cpp'),
]

if enable_rapl
  lib_efimon_sources += [
    files('power/rapl.cpp'),
  ]
endif

if enable_perf
  lib_efimon_sources += [
    files('perf/record.cpp'),
    files('perf/annotate.cpp'),
  ]
endif

if enable_pcm
  lib_efimon_sources += [
    files('power/intel.cpp'),
  ]
endif

if enable_sql
  lib_efimon_sources += [
    files('logger/sqlite.cpp'),
  ]
endif

if enable_ipmi and enable_ipmi_sensors
  lib_efimon_sources += [
    files('power/ipmi.cpp'),
  ]
endif
