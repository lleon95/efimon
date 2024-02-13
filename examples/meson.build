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
           install : true,
)


executable('sqlite-testing',
           [
             files('sqlite-testing.cpp')
           ],
           cpp_args : cpp_args,
           include_directories : [project_inc],
           dependencies: [libefimon_dep],
           install : true,
)

executable('perf-testing',
           [
             files('perf-testing.cpp')
           ],
           cpp_args : cpp_args,
           include_directories : [project_inc],
           dependencies: [libefimon_dep],
           install : true,
)
