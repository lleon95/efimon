#
# See LICENSE for more information about licensing
#  Copyright 2023-2024
#
# Author: Luis G. Leon Vega <luis.leon@ieee.org>
#

lib_efimon_sources += [
  files('arg-parser.cpp'),
  files('proc/io.cpp'),
  files('proc/meminfo.cpp'),
  files('proc/net.cpp'),
  files('proc/stat.cpp'),
  files('proc/thread-tree.cpp'),
  files('proc/process-tree.cpp'),
  files('uptime.cpp'),
  files('asm-classifier.cpp'),
  files('asm-classifier/x86-classifier.cpp'),
  files('proc/cpuinfo.cpp'),
  files('process-manager.cpp'),
  files('logger/csv.cpp'),
]

if enable_libprocps
  lib_efimon_sources += [
    files('proc/list.cpp'),
  ]
endif

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

if enable_ptrace_capstone
  lib_efimon_sources += [
    files('ptrace-capstone/ptrace-capstone.cpp'),
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

if enable_sampling_by_pid
  sampling_by_pid_dir = 'ebpf-modules/cpu-assembly-sampler'

  sampling_by_pid_vmlinux = custom_target('sampling_by_pid_vmlinux_h',
  command: [
    'sh', '-c',
		'bpftool btf dump file /sys/kernel/btf/vmlinux format c > @OUTPUT@'
  ],
	output: 'vmlinux.h',
  )

  sampling_by_pid_bpf_obj = custom_target('sampling_by_pid_prog_bpf_o',
    input: files(sampling_by_pid_dir / 'prog.bpf.c'),
    output: 'prog.bpf.o',
    command: [
      clang,
      '-target',
      'bpf',
      '-D__TARGET_ARCH_x86',
      '-O2',
      '-g',
      '-I',
      join_paths(meson.current_source_dir(), sampling_by_pid_dir),
      '-I',
      join_paths(meson.current_build_dir(), sampling_by_pid_dir),
      '-I',
      meson.current_build_dir(),
      '-c',
      '@INPUT@',
      '-o',
      '@OUTPUT@',
    ],
    depends: sampling_by_pid_vmlinux,
  )

  sampling_by_pid_skeleton = custom_target('sampling_by_pid_prog_skel_h',
    input: sampling_by_pid_bpf_obj,
    output: 'prog.skel.h',
    command: [bpftool, 'gen', 'skeleton', '@INPUT@'],
    capture: true,
    build_by_default: true,
  )

  sampling_by_pid_skeleton_dep = declare_dependency(
    include_directories: include_directories('.', sampling_by_pid_dir),
    sources: sampling_by_pid_skeleton,
  )

  lib_efimon_extra_deps += [sampling_by_pid_skeleton_dep]

  lib_efimon_sources += [
    files('ebpf-modules/cpu-assembly-sampler/sampling-by-pid.cpp'),
  ]
endif
