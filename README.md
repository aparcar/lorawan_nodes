
## Payload information

| JSON value      | description                        |
| ---             | ---                                |
| `analog_in_1`   | rain mm/m                          |
| `analog_in_2`   | soil moisture                      |
| `digital_out_1` | running software version (integer) |
| `temperature_1` | temperature                        |
| `voltage_1`     | battery voltage                    |

## active nodes

| verbose name | ID               |
| ---          | ---              |
| rain-box-1   | 00253073E3D8A429 |
| rain-box-2   | 2039823098092384 |
| rain-box-3   | 013298498FA9DFAE |
| rain-box-4   | 230948230598E9AD |
| rain-box-5   | 23094809F8DF0982 |
| rain-box-6   | E8743987987DFEAD |
| temp-box-1   | 1230918230912830 |

## versions

The `digital_out_1` value contains an integer representing the running software
version on a node. See changes below.

### Version 3

* Add **VH400** sensor
* More modular enabling of sensors
