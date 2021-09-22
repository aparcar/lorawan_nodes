# Sensors

This section describes briefly sensors used within this project. All of them are
supported by the *Arduino* framework an can be connected in any variation to the
described [microcontrollers](../hardware/microcontroller.md).

## Distance

To measure the distance between a fixed point and sea level the ultrasonic
**MB7389** sensor from *MaxBotix Inc.* was used. It offers a distance range of
`300mm` to `5000mm` (`5m`). Measurements happen 6 times per second and are
readable via an analog, pulse width or serial output. The easiest implementation
is done by using [Arduinos `softwareserial`][softwareserial] and read the
distance values as chars. A full data sheet is available on the [vendors
website][distance].

[softwareserial]: https://www.arduino.cc/en/Reference/SoftwareSerialConstructor
[distance]: https://www.maxbotix.com/documents/HRXL-MaxSonar-WR_Datasheet.pdf

## Rain fall

For rain fall two different rain gauges are used. While one is the de-facto
standard for scientific publications, the other allows measurements at a much
lower price. Both are usable by using a simple tip counter via an `GPIO`
interrupt and are simply connected to a `GND` PIN and whichever PIN was
configured with the interrupt.

### ONSET HOBO RG-3

The **HOBO Rain Gauge** by *Onset Computer Corporation* offers a high quality
metal casing and is described as the default rain gauge for scientific
publications. With it's precision comes the price of [around $400][hobo] which
may exceed the budget for some use cases.


> One tip means `0.254mm` or `0.1"` of rainfall.

[hobo]: https://www.onsetcomp.com/products/data-loggers/rg3/

### Misol WH-SP-RG

An alternative to is the **WH-SP-RG** by *Misol*. At the time of writing
(2021-09-21) the vendors website is not reachable, however other websites sell
the rain gauge for [around $20][misol].

> One tip means `0.3851mm` of rainfall.

[misol]: https://www.amazon.com/MISOL-Spare-weather-station-measure/dp/B00QDMBXUA/

## Soil Moisture

For soil moisture the **VH400** by *Vegetronix, Inc* is supported. The sensor
is connected to an `ADC` (*Analog Digital Converter*) PIN and the measured
resistance describes the *volumetric water content* by using a [piecewise
curve][curve] provided by the vendor. The curve is implemented in the [provided
mini library](../../node/lib/VH400/VH400.cpp). The sensor costs [around
$40][vh400].

[curve]: https://vegetronix.com/Products/VH400/VH400-Piecewise-Curve.phtml
[vh400]: https://vegetronix.com/Products/VH400/

## Temperature

For waterproof temperature measuring a **DS18B20** sold by *Adafruit Industries,
LLC* is used. Adafruit ships their own libraries so minimal integration needs to
be coded. The sensor uses three wires which can be directly connected to the
[custom PCB](../hardware/pcb.md). The sensor costs [ardoung $10][temperature] on
the Adafruit website.

[temperature]: https://www.adafruit.com/product/381
