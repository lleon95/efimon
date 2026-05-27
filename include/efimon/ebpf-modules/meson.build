#
# See LICENSE for more information about licensing
#  Copyright 2026
#
# Author: Diego Avila <diego.avila@uned.cr>
#

# eBPF Module: Sampling by PID
subdir('sampling-by-pid')

# eBPF Module: Disk I/O Tracing
subdir('disk-io')

# Concatenate all eBPF modules headers
lib_ebpf_modules_headers = []
lib_ebpf_modules_headers += lib_sampling_by_pid_headers
lib_ebpf_modules_headers += lib_disk_io_headers
