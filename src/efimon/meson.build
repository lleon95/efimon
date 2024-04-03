#
# See LICENSE for more information about licensing
#  Copyright 2023
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
  files('perf/record.cpp'),
  files('perf/annotate.cpp'),
  files('uptime.cpp'),
  files('asm-classifier.cpp'),
  files('asm-classifier/x86-classifier.cpp'),
  files('proc/cpuinfo.cpp'),
  files('power/rapl.cpp'),
]

if get_option('enable-pcm')
  lib_efimon_sources += [
    files('power/intel.cpp')
  ]
endif

if sqlite_dep.found()
  lib_efimon_sources += [
    files('logger/sqlite.cpp'),
  ]
endif
