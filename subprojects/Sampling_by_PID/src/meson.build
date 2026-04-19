bpf_o = 'prog.bpf.o'
bpf_skel_h = 'prog.skel.h'

# Generate vmlinux.h first
vmlinux = custom_target('vmlinux',
  command: [
    'sh', '-c',
		'bpftool btf dump file /sys/kernel/btf/vmlinux format c > @OUTPUT@'
  ],
	output: 'vmlinux.h',
)


# Compile eBPF program
bpf_prog = custom_target('bpf_prog',
	input: 'prog.bpf.c',
	output: bpf_o,
	command: [
	clang,
	'-target', 'bpf',
	'-D__TARGET_ARCH_x86',
	'-O2', '-g',
		'-I', meson.current_source_dir(),
		'-I', meson.current_build_dir(),
		'-c', '@INPUT@',
		'-o', '@OUTPUT@'],
		depends: vmlinux
)

# Generate skeleton from eBPF object
skeleton = custom_target('bpf_skeleton',
	command: ['bpftool', 'gen', 'skeleton', '@INPUT@'],
	input: bpf_prog,
	output: bpf_skel_h,
	capture: true,
	build_by_default: true
)

# Create a dependency on the skeleton generation
skeleton_dep = declare_dependency(
	include_directories: include_directories('.'),
	sources: skeleton
)

# Reusable sampler library (replace main-based executable)
sampler_lib = static_library('cpu_sampler',
	'sampler_lib.cpp',
	include_directories: [projectinc, include_directories('.' / '..' / 'include')],
	dependencies: [project_deps, skeleton_dep],
	cpp_args: ['-O0', '-g', '-Wall'],
	install: true
)

sampler_dep = declare_dependency(
	link_with: sampler_lib,
	include_directories: [projectinc, include_directories('.' / '..' / 'include')],
	dependencies: [project_deps, skeleton_dep]
)

install_headers('../include/sampler_lib.hpp', subdir: 'cpuconsumptionmodule')

# Benchmark executable
benchmark_exe = executable('bench',
	'benchmark.c',
	c_args: ['-O0', '-I/usr/include/openblas'],
	link_args: ['-lopenblas', '-lm'],
	install: true
)
