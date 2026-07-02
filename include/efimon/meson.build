#
# See LICENSE for more information about licensing
#  Copyright 2023
#
# Author: Luis G. Leon Vega <luis.leon@ieee.org>
#

lib_iface_headers = [
  files('arg-parser.hpp'),
  files('asm-classifier.hpp'),
  files('logger.hpp'),
  files('observer.hpp'),
  files('observer-enums.hpp'),
  files('proc-lister.hpp'),
  files('process-manager.hpp'),
  files('readings.hpp'),
  files('status.hpp'),
]

install_headers(lib_iface_headers, subdir : 'efimon')

# -----------------------------------------------------------------------------
# Other headers
# -----------------------------------------------------------------------------

# ASM Classifier Specifics
subdir('asm-classifier')

# Logger Specifics
subdir('logger')

# ASM Analysers
subdir('perf')
subdir('ptrace-capstone')

# eBPF Modules
subdir('ebpf-modules')

# Power Specifics
subdir('power')

# Proc Specifics
subdir('proc')

# Reading Specifics
subdir('readings')

# Concatenate all
lib_headers = lib_iface_headers
lib_headers += lib_asm_classifier_headers
lib_headers += lib_logger_headers
lib_headers += lib_perf_headers
lib_headers += lib_ptrace_capstone_headers
lib_headers += lib_ebpf_modules_headers
lib_headers += lib_power_headers
lib_headers += lib_proc_headers
lib_headers += lib_readings_headers

install_headers(lib_asm_classifier_headers, subdir : 'efimon/asm-classifier')
install_headers(lib_logger_headers, subdir : 'efimon/logger')
install_headers(lib_perf_headers, subdir : 'efimon/perf')
install_headers(lib_ptrace_capstone_headers, subdir : 'efimon/ptrace-capstone')
install_headers(lib_sampling_by_pid_headers, subdir : 'efimon/ebpf-modules/cpu-assembly-sampler')
install_headers(lib_disk_io_headers, subdir : 'efimon/ebpf-modules/disk-io')
install_headers(lib_power_headers, subdir : 'efimon/power')
install_headers(lib_proc_headers, subdir : 'efimon/proc')
install_headers(lib_readings_headers, subdir : 'efimon/readings')
