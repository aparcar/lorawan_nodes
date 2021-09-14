# CayenneLPP

This section explain CayenneLPP (Low Power Protocol) which is used to encode
data on sensor nodes into a size efficient format and decode it in the backend
back for further processing.

*Cayenne* is a platform developed by *myDevices Inc.* offering custom dashboards
for device monitoring, alerts and more[^mydevices]. They developed
CayenneLPP[^llp] and multiple open source implementation of that protocol
exists[^cats][^py]. The protocol allows a compromise between self describing
data packets and size efficiency, which will be explained in the next section.

This sections below show three ways to encode node data to a binary format for
sending over LoRa, a custom *raw* format, *JSON* and CayenneLPP. The example
data is a **temperature** value of 18.6 degree Celsius, a **battery voltage** of
3.9V and a **distance** value of 43mm.

## Encoding using raw Bits

The smaller a LoRaWAN packet is the less power is required to send it and the
more airtime is available for other sensors. A simple solution would be just
store all sensor data as binary fields one after another. The packet could have
the following format:

```
<temperature><voltage><distance>
```

Representing floats in binary isn't trivial to instead a prevision can be
defined. For **temperature** two float digits are enough, for **battery
voltage** a single float digit is fine and **dinstance** is always a integer.

This means values can be multiple by `100` respectively `10`. The temperature value
becomes `1860` and the voltage becomes `39`. Both considering that measured
temperature will never be above 99.99 degree Celsius and batter voltage should
exceed 9.9V (4.2V really). For distance a maximum of 5000mm should be used.

To calculate the maximum bits needed data, the maximum values are converted to
binary format the bit counted, easily possible with some Python:

```python3
>>> format(9999, "b") # max value
'10011100001111'
>>> len(format(9999, "b"))
14
>>> format(99, "b") # max value
'1100011'
>>> len(format(99, "b"))
7
>>> format(5000, "b") # max value
'1001110001000'
>>> len(format(5000, "b"))
13
>>> 
```

Above calculation shows that **14** Bits are needed for temperature, **7** for
battery voltage and **13** for distance. Now the custom data format becomes more
specific as shown below:

```
| temperature 14 | volt 7  | distance 13   |
| xxxxxxxxxxxxxx | xxxxxxx | xxxxxxxxxxxxx |
```

Below we calculate the specific values and pad them with zeros.

```python3
>>> format(1860, "b")
'11101000100'
>>> format(39, "b")
'100111'
>>> format(43, "b")
'101011'
```

The resulting packet would look like this with a total size of **34 Bits**.

```
| temperature 14 | volt 7  | distance 13   |
| 00011101000100 | 0101011 | 0000000101011 |
```

The receiving backend could decode this data by reading the first 14 Bits as
integer and dividing the value by 100 to calculate the temperature, followed by
reading Bit 15 until 21 as integer and dividing it by 10 to calculate the
battery voltage. The remaining 13 Bit can be directly read as distance.

While this approach works and data is efficiently exchanged between sensor node
and backend, the calculation is not trivial to understand. More problematic is
the extendability of this approach: Newly added sensors may use a different
set of sensors and therefore require data fields. As the exchanged data is just
Bits, it's not possible to know what the data contains. In other words, the
format is extremely static and ideally the data would be more self explaining so
a backend would know what value it is decoding.

## Encoding using JSON

The JSON format is extremely popular in web application to exchange all kinds of
data. It is human readable, supports all our data types directly (float and
integer) and allows to verbosely describe data fields. The example data could be
decoded as below:

```json
{
	"temperature_1": 18.6,
	"voltage_1": 3.9,
	"distance_1": 43
}
```

As printed above, the format would take a total of 73 Bytes (584 Bits) which is
nearly 20 times bigger than using the raw format. Even a slightly optimized
version as shown below would still require 28 Bytes (224 Bits) meaning 7 times
bigger.

```json
{"t1":18.6,"v1":3.9,"d1":43}
```

## Encoding using CayenneLPP

CayenneLPP uses a compromise of both approaches, the data is very compact while
being self descriptive. To archive that common measurement types are available
with a 1 Byte data type descriptor and a 1 Byte data type channel (or ID). Each
data type, be it temperature, GPS or relative humidity is described by a number
between 0 and 255, a official reference implementation is available in the
[Cayenne Docs][cayenne-docs]. Since all data types are based on [IPSO data
types][ipso] other implementations support additional values, like voltage,
altitude and distance[^cats-api].

Adding those values on device is done by a creating a `CayenneLPP` frame and
adding values. More details are available in [`main.cpp`
implementation](node/main.cpp.md), however below is an simplified example.

```C
CayenneLPP lpp(LORAWAN_APP_DATA_MAX_SIZE);      // create frame
lpp.addVoltage(1, getBatteryVoltage());         // add voltage
lpp.addDistance(1, distance);                   // add distance in mm
lpp.addTemperature(1, temperature);             // add temperature in Celsius
appDataSize = lpp.getSize();                    // calculate size
memcpy(appData, lpp.getBuffer(), appDataSize);  // copy Bytes to LoRaWAN packet
```

The same is possible to do via Python using the `pycayennelpp` package[^py]:

```python
from cayennelpp import LppFrame

frame = LppFrame()
frame.add_temperature(0, 18.6)
frame.add_distance(1, 43)
frame.add_voltage(1, 3.9)
buffer = bytes(frame)
print(len(buffer))
```

The result are **14 Bytes** and thereby half the size of using JSON. For
measurements which are not directly supported by the implementations, like
*number of satelites* for GPS measurements, it is possible to use the commands
`addDigitalInput` or `addDigitalOutput` (unsigned  32 Bit Integer) and
`addAnalogInput` or `addAnalogOutput` (float with 3 decimals).

## Decoding in Backend

The receiving backend allows to decode incoming data before offering it via
MQTT. This is very useful so different applications listening to the MQTT stream
can directly process the decoded payload rather than implementing that per
client, allowing as well to upgrade the communication used for nodes without
modifying clients.

TheThingsNetwork offers to automatically decode CayenneLPP frames using the
*Payload Formatter* menu entry.

![](/img/ttn_cayenne.png)

However, since TheThingsNetwork uses the reference implementation of *MyDevices*
they don't support decoding some additional data types, like distance. If these
measurements are used it is possible to use a custom JavaScript decoder as
offered by *ElectronicCats* to be used directly in TheThingsNetwork. The
[`decoder.min.js`][decoder] can be pasted into the text field of the *Payload
Formatter* menu entry:

> The `min` decoder version is required as TheThingsNetwork limits the total
> size of decoding scripts and the formated version exceeds that limit.

![](/img/ttn_javascript.png)

[^mydevices]: https://developers.mydevices.com/cayenne/features/
[^llp]: https://developers.mydevices.com/cayenne/docs/lora/#lora-cayenne-low-power-payload
[^cats]: https://github.com/ElectronicCats/CayenneLPP/
[^cats-api]: https://github.com/ElectronicCats/CayenneLPP/blob/master/API.md#methods-add
[^py]: https://github.com/smlng/pycayennelpp
[cayenne-docs]: https://developers.mydevices.com/cayenne/docs/lora/#lora-cayenne-low-power-payload-reference-implementation-cayenne-lpp-cc-constants-definitions
[ipso]: https://technical.openmobilealliance.org/OMNA/LwM2M/LwM2MRegistry.html#extlabel
[decoder]: https://github.com/ElectronicCats/CayenneLPP/tree/master/decoders
