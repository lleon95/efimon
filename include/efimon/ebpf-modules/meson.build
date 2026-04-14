#
# See LICENSE for more information about licensing
#  Copyright 2024
#
# Author: Luis G. Leon Vega <luis.leon@ieee.org>
#

# eBPF Module: Sampling by PID
subdir('sampling-by-pid')

# eBPF Module: ModuloIODisk
subdir('moduloio-disk')

# Concatenate all eBPF modules headers
lib_ebpf_modules_headers = []
lib_ebpf_modules_headers += lib_sampling_by_pid_headers
lib_ebpf_modules_headers += lib_moduloio_disk_headers
