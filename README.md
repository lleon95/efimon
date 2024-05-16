# EfiMon: An agnostic efficiency monitor library

Easy-To-Use Linux Power and Resource Consumption Monitoring Library and Daemon. Monitors CPU, GPU, RAM, I/O, Network and the overall system.

## Installation

This library uses `meson` as the building system. To install meson, Python >= 3.8 is required. Please, check the following instructions according to your needs:

* Python and PIP

```bash
# Ubuntu
sudo apt install python3 python3-pip
```

* Ninja Build

```bash
# Ubuntu
sudo apt install ninja-build
```

* Meson

```bash
sudo pip3 install meson
```

**Dependencies**

First, it is important to recall that this is a Linux library. It requires the following dependencies:

* libprocps
* sqlite3 >= 3.31.1
* Linux Perf
* Intel RAPL
* Free IPMI

On Fedora 16, you can install some of these dependencies using:

```bash
# ProcPS
yum install procps-ng-devel.x86_64 procps-ng.x86_64
# Linux Perf
yum install perf
# SQLite
yum install libsqlite3x.x86_64 libsqlite3x-devel.x86_64
```

On Ubuntu 20.04:

```bash
# ProcPS
apt install libprocps-dev libprocps8
# Linux Perf
apt install linux-tools-common linux-tools-generic
# SQLite
apt install libsqlite3-dev libsqlite3-0
```

**Optional**

For the documentation, you will require other dependencies like graphviz, java, texlive and doxygen. Please, install them by using:

```bash
# Ubuntu
sudo apt install doxygen graphviz openjdk-17-jre texlive-font-utils
```

You can switch the "-Dbuild-docs" or "-Dbuild-docs-only" to true if you want to compile the documentation.

### Compiling EfiMon

Compiling EfiMon follows the same process as any meson project. Please, follow the steps to have EfiMon on your system:

```bash
meson builddir $OPTIONS
ninja -C builddinr
sudo ninja -C builddir
```

EfiMon will be installed under the `/usr/local/` prefix.

For the `$OPTIONS`, you can select the following:

| Option                 | Default Value    | Values        | Description                      |
|------------------------|------------------|---------------|----------------------------------|
| build-docs             | false            | [true, false] | Enable docs compilation          |
| build-docs-only        | false            | [true, false] | Enable docs-only compilation     |
| build-examples         | true             | [true, false] | Enable examples compilation      |
| developer-mode         | true             | [true, false] | Enable developer mode            |
| enable-pcm             | true             | [true, false] | Enable the Intel PCM             |
| enable-perf            | true             | [true, false] | Enable the Linux Perf Tool       |
| enable-rapl            | true             | [true, false] | Enable the RAPL Interface        |
| enable-sql             | true             | [true, false] | Enable the SQL Logger            |
| enable-ipmi            | true             | [true, false] | Enable the IPMI Logger           |

### Testing the installation

T.B.D


## Usage

Currently, we have been focused on an analyser for measuring the power consumption based on the histograms.

```bash
PID=2000 # Process ID
STIME=10 # Metering during 10 seconds
sudo efimon-power-analyser -p ${PID} -s ${STIME} > consumption.csv
```

Another way to execute it:

```bash
STIME=10 # Metering during 10 seconds
# the command is time sleep 1
sudo efimon-power-analyser -s ${STIME} -c time sleep 1
```

## Platforms

EfiMon has been tested in the following platforms:

* Ubuntu 20.04 with Intel Core i5 6000
* Fedora 16 with AMD Epyc Zen2

## Additional Information

This project is given to you under the LGPL v2.1 Licence. Dynamic linkage from commercial applications to EfiMon is allowed. Derived work and modifications must be released under the same license as EfiMon (LGPL v2.1).

### Contributors

* Luis G. Leon Vega <luis.leon@ieee.org>

### Official Repository

* Github: https://github.com/lleon95/efimon
