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
  files('logger/sqlite.cpp'),
  files('perf/record.cpp'),
  files('perf/annotate.cpp'),
  files('uptime.cpp'),
  files('asm-classifier.cpp'),
  files('asm-classifier/x86-classifier.cpp'),
]

if get_option('enable-pcm')
  lib_efimon_sources += [
    files('power/intel.cpp')
  ]
endif
