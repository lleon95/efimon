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
meson builddir
ninja -C builddinr
sudo ninja -C builddir
```

EfiMon will be installed under the `/usr/local/` prefix.

### Testing the installation

T.B.D


## Usage

T.B.D

## Additional Information

This project is given to you under the LGPL v2.1 Licence. Dynamic linkage from commercial applications to EfiMon is allowed. Derived work and modifications must be released under the same license as EfiMon (LGPL v2.1).

### Contributors

* Luis G. Leon Vega <luis.leon@ieee.org>

### Official Repository

* Github: https://github.com/lleon95/efimon
