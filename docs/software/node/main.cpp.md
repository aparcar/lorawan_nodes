# main.cpp

This section describes the modular node which allows to collect and send
environmental metrics to a backend. While it is possible to use pure *LoRa* to
communicate with a backend, the code provided here uses only *LoRaWAN*. While a
specific hardware was used to build the nodes, it is possible to replace the
transmission logic with other libraries.

All code described here is found in `node/src/main.cpp`. Below is a highlighted
version of the code fragment 

```cpp
--8<-- "node/src/main.cpp"
```
