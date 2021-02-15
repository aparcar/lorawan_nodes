# Nodes configuration

The script `provision_node.py` allows to setup a node via AT commands. These
commands are used to provision devices, set LoRaWAN credentials or adjust
sensors. Supported commands vary per implementation, the basic commands for
LoRaWAN connectivity are always supported.

The file `rain-1.yml` is an illustrative node configuration. To setup a
microcontroller with the `rain-1` configuration run the following command:

```shell
python3 provision_node.py rain-1
```

## Commands

A list of supported commands.

| Command    | Example                            | Content        | Note     |
| ---        | ---                                | ---            | ---      |
| `DevEui`   | `0000000000000000`                 | LoRaWAN DevEui | required |
| `AppEui`   | `FFFFFFFFFFFFFFFF`                 | LoRaWAN AppEui | required |
| `AppKey`   | `11111111111111111111111111111111` | LoRaWAN AppKey | required |
| `mmPerTip` | `0.254`           | mm (metric) per tip | Only for rain gauges |
