# Overview

This section describes the software used on nodes. Within this overview page
information on the usage of [Platformio] and provisioning via the `provision.py`
script is given.


## On device code

For further information on the C code see the following sections:

* [config.h] Contains node specific configuration options, like what sensors to
  enable on what PINs. This should be modified per use case, for instance when
  creating a new series of rain gauges measuring moisture as well, the sensor
  should be enabled here. Per node configuration happens via the `provision.py`
  script after flashing.
* [main.cpp] Contains the main loop which connects to sensors and sends LoRaWAN
  packets.
* [VH400.cpp] Minimal library to communicate with *VH400* moisture sensor.
* [MB7389.cpp] Minimal library to communicate with *MB7389* ultrasonic sensor.




[platformio]: https://platformio.org/
[config.h]: config.h
[main.cpp]: main.cpp
[VH400.cpp]: VH400.cpp
[MB7389.cpp]: MB7389.cpp
