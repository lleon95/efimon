#
# See LICENSE for more information about licensing
#  Copyright 2023-2024
#
# Author: Luis G. Leon Vega <luis.leon@ieee.org>
#

executable('process-tracking',
           [
             files('process-tracking.cpp')
           ],
           cpp_args : cpp_args,
           include_directories : [project_inc],
           dependencies: [libefimon_dep],
           install : false,
)

if enable_sql
  executable('sqlite-testing',
            [
              files('sqlite-testing.cpp')
            ],
            cpp_args : cpp_args,
            include_directories : [project_inc],
            dependencies: [libefimon_dep],
            install : false,
  )
endif

if enable_perf
  executable('perf-testing',
            [
              files('perf-testing.cpp')
            ],
            cpp_args : cpp_args,
            include_directories : [project_inc],
            dependencies: [libefimon_dep],
            install : false,
  )
endif

if enable_rapl
  executable('rapl-testing',
            [
              files('rapl-testing.cpp')
            ],
            cpp_args : cpp_args,
            include_directories : [project_inc],
            dependencies: [libefimon_dep],
            install : false,
  )
endif

if enable_ipmi
  executable('ipmi-testing',
            [
              files('ipmi-testing.cpp')
            ],
            cpp_args : cpp_args,
            include_directories : [project_inc],
            dependencies: [libefimon_dep],
            install : false,
  )
endif

if enable_pcm
  executable('pcm-testing',
            [
              files('pcm-testing.cpp')
            ],
            cpp_args : cpp_args,
            include_directories : [project_inc],
            dependencies: [libefimon_dep],
            install : false,
  )
endif

executable('process-manager',
          [
            files('process-manager.cpp')
          ],
          cpp_args : cpp_args,
          include_directories : [project_inc],
          dependencies: [libefimon_dep],
          install : false,
)

executable('process-manager-threaded',
          [
            files('process-manager-threaded.cpp')
          ],
          cpp_args : cpp_args,
          include_directories : [project_inc],
          dependencies: [libefimon_dep, dependency('threads')],
          install : false,
)

executable('frequency-query',
          [
            files('frequency-query.cpp')
          ],
          cpp_args : cpp_args,
          include_directories : [project_inc],
          dependencies: [libefimon_dep],
          install : false,
)

executable('csv-testing',
          [
            files('csv-testing.cpp')
          ],
          cpp_args : cpp_args,
          include_directories : [project_inc],
          dependencies: [libefimon_dep],
          install : false,
)

if enable_nvidia_dgpu
  executable('nvidia-ml-measurement',
            [
              files('nvidia-ml-measurement.cpp')
            ],
            cpp_args : cpp_args,
            include_directories : [project_inc],
            dependencies: [libefimon_dep, project_deps, dependency('threads')],
            install : false,
  )
endif

executable('ptx-interpreter',
          [
            files('ptx-interpreter.cpp')
          ],
          cpp_args : cpp_args,
          include_directories : [project_inc],
          dependencies: [libefimon_dep],
          install : false,
)
