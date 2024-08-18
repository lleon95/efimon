#
# See LICENSE for more information about licensing
#  Copyright 2023
#
# Author: Luis G. Leon Vega <luis.leon@ieee.org>
#

executable('efimon-meter',
           [
             files('efimon-meter.cpp')
           ],
           cpp_args : cpp_args,
           include_directories : [project_inc],
           dependencies: [libefimon_dep],
           install : true,
)

executable('efimon-power-analyser',
          [
            files('efimon-power-analyser.cpp')
          ],
          cpp_args : cpp_args,
          include_directories : [project_inc],
          dependencies: [libefimon_dep, dependency('threads')],
          install : true,
)

if enable_zeromq and enable_jsoncpp
  executable('efimon-daemon',
            [
              files(
                'efimon-daemon.cpp',
                'efimon-daemon/efimon-analyser.cpp',
                'efimon-daemon/efimon-worker.cpp',
              )
            ],
            cpp_args : cpp_args,
            include_directories : [project_inc],
            dependencies: [project_deps, libefimon_dep, dependency('threads')],
            install : true,
  )
endif
