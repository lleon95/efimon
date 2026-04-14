# eBPF Sampler & Benchmark Project
Este proyecto implementa un sistema de muestreo de rendimiento basado en **eBPF**.
Permite monitorear procesos en ejecución, recolectar métricas y evaluar el impacto (slowdown) sobre el rendimiento mediante un **benchmark** y visualización de resultados.

---

## Estructura del proyecto
EfiMon.CPUConsumptionModule/
│
├── src/
│   ├── main.cpp                              # Programa principal en espacio de usuario
│   ├── prog.bpf.c                            # Programa eBPF (kernel space)
│   └── benchmark.c                           # Código C para generar carga (multiplicación de matrices)
│
├── include/
│   ├── vmlinux.h                             # Definición de estructuras del kernel (generado)
│   └── prog.skel.h                           # Skeleton autogenerado por bpftool
│
├── build/
│   ├── prog.bpf.o                            # Objeto compilado del eBPF
│   ├── main                                  # Binario compilado del programa en user-space
│   └── benchmark                             # Binario compilado del benchmark
│
├── data/
│   ├── results/
│   │   ├── bench_no_sampler.out              # Archivos de salida de las pruebas
│   │   ├── bench_with_sampler.out 
│   │   └── results.csv                       # Resultados crudos del benchmark
│   └── plots/
│       └── slowdown_vs_freq.png     # Imagen del plot con los resultados de las pruebas
│
├── scripts/
│   └── run_tests.sh                          # Script para ejecutar el benchmark
│   └── analyze_results.py                    # Script para generar gráficas con matplotlib
│
├── Makefile                                  # Archivo CMake para compilación y ejecución
└── README.md                                 # Instrucciones generales del proyecto

---

## Dependencias
Asegúrate de tener instaladas las siguientes herramientas antes de compilar:

### Paquetes del sistema
```bash
sudo apt update
sudo apt install clang llvm libbpf-dev libelf-dev zlib1g-dev gcc g++ bpftool python3-matplotlib python3-pandas libopenblas-dev
```

### Requisitos adicionales
- Kernel con soporte eBPF (5.8+ recomendado)
- Permisos para ejecutar bpftool y cargar programas eBPF (usualmente requiere sudo)

## Compilación
Para construir todos los componentes (eBPF, sampler, benchmark y skeleton):
```bash
make all
```

Esto generará:
- include/vmlinux.h — definición del kernel
- build/prog.bpf.o — bytecode eBPF
- include/prog.skel.h — skeleton generado automáticamente
- build/sampler — programa en espacio de usuario
- build/benchmark — programa de benchmark

## Ejecución
1. Ejecutar el sampler sobre un proceso
Para monitorear un proceso específico (por su PID):
```bash
sudo make run PID=<PID> F=<FREQUENCY> D=<DURATION>
```

2. Ejecutarlo de forma manual
```bash
./build/sampler <PID> <FREQUENCY> <DURATION>
```

## Ejecución de pruebas
El proyecto incluye un script que ejecuta un benchmark:
```bash
make test
```

Este guarda los resultados en: data/results.csv

**NOTA: Para ver y obtener mejores resultados (principalmente en el plot), se recomienda ejecutar las pruebas varias veces con distintas frecuencias de muestreo. Puedes modificar el valor de FREQ editando el archivo: scripts/run_tests.sh**

## Visualización de resultados
Para generar los gráficos:
```bash
make plot
```

El script scripts/analyze_results.py lee el archivo CSV con los resultados y produce una gráfica slowdown vs frequency en:
```bash
data/plots/
```

## Limpieza
Para eliminar todos los archivos generados (builds, skeletons, etc.):
```bash
make clean
```

## Tecnologías utilizadas
- eBPF / libbpf: recolección eficiente de métricas desde el kernel
- C / C++: componentes en espacio de usuario y benchmark
- bpftool: generación automática de headers (vmlinux.h, skeleton)
- Python (matplotlib, pandas): visualización de resultados
- OpenBLAS: operaciones de cómputo intensivo en el benchmark

