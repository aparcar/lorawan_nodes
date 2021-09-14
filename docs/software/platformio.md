# Platformio

This section describes the usage of the *Platformio* framework for embedded
development. Similar to the *Arduino IDE*[^ide] it allows the management of
dependencies and compilation for a wide variety of devices.

Due to its simple installation and full support for the used hardware it was
preferred over other the *Arduino IDE*, however other tooling should be possible
as well.

## Installation

A full installation guide is available in the [upstream documentation][pio]
however the basic installation boils down to a running Python 3 installation
combined with the Python Packet Manager or `curl`:

```bash
pip install -U platformio

# or

python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio/master/scripts/get-platformio.py)"
```

After the installation the shortcut `pio` executes the binary.

## Creating a project

Adding projects is as easy as creating a folder and running `pio` to initialize
the project in the current folder.

```bash
mkdir project/ && cd project/
pio project init
```

After the initialization the following files are created:

```bash
include/       # header files
lib/           # libraries
platformio.ini # platformio configuration file
src/           # source code (e.g. main.cpp)
test/          # automatic tests
```

See the Git repository folder `node/` to see the main project structure.

## platformio.ini

The configuration file contains one or multiple environments per used device
type. Within this projects environments for the devices `cube_cell_board`,
`cubecell_board_plus` and `cubecell_module_plus` are used. The file below
defines multiple aspects or the project which simplify development:

* `board_build.ardiono.*` defines values passed during compile time to enable
  and configure specific features.
* `upload_port` defines the local connection, which allows to have multiple
  devices connected at once and test them in parallel
* `lib_deps` defines all required dependencies which can be hosted within the
  Platformio registry[^registry] or Git repositories directly.

```ini
--8<-- "node/platformio.ini"
```

## Flashing nodes

Once a sensor node is connected it's can be flashed using the following command:

```shell
pio run --target upload
```

For convenience it is also possible to upload and monitor the serial output at
once:

```shell
pio run --target upload --target monitor
```

[^ide]: https://www.arduino.cc/en/software/
[pio]: https://docs.platformio.org/en/latest//core/installation.html
[^registry]: https://platformio.org/lib
