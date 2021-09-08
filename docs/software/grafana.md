# Grafana Dashboard

Presenting collected data is important to make them accessible and give an
overview of events before starting in-depths analytics. While multiple
approaches to visualize data exists, this section describes the setup of
Grafana.

The advantages of Grafana compared to other solutions is the fact that without
any coding skills appealing graphs and gauges can be created, offering a live
and interactive user interface.

Within this setup the combination with the [InfluxDB database](influxdb.md) is
described, however it is possible to visualize [other data sources][datasources]
via Grafana.

[datasources]: https://grafana.com/docs/grafana/latest/datasources/

## Installation

To allow a cheap and reproducible setup these steps are done on a RaspberryPi
running [Raspbian], which allows low cost data hosing without requiring any
server setups. However it is equally possible to run this on a virtual machine
or even a laptop for testing.

The Raspbian ports of Grafana are outdated and therefore the official
Grafana repository is used instead:

```bash
echo "deb https://packages.grafana.com/oss/deb stable main" | \docs
	sudo tee -a /etc/apt/sources.list.d/grafana.list 
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
sudo apt update
sudo apt install -y grafana-rpi
```

More details on the Grafana installation steps are available in the official
[documentation][grafana-debian].

## Adding InfluxDB as data source

TODO

## Configure a graphs

TODO

## Example Dashboard

Below is a screenshot of a demo setup which includes two graphs of measured
metrics (rain fall and temperature) and below an overview of the nodes over the
last week. It is possible to interactively select smaller and bigger time frames
and see precise values for specific times and dates.

![](/img/grafana_rain_temperature.png)

Aboves two graphs show rain events that happened at different location in
Honolulu, Hawaii. It's possible to zoom into specific time ranges to see the
total amount of rain for that day, hour or minute.

The temperature graph below the *rain fall* shows a repeating pattern of
temperature changes. Two special cases are handled in this graph showing where
`rain-box-1` shows the *inner enclosing temperature* and `rain-box-6` is
stationed in a cooled lab, therefore the constantly low temperature.

Additional values can be measured to track the *health* of sensors, most
importantly the battery voltages and connection quality (*RSSI*).

![](/img/grafana_voltage.png)


The graph above shows how the battery voltage is dropping every night but
recharged on sunrise.

![](/img/grafana_voltage_low.png)

On the contrary, above graphs show that the node `sonic-2` doesn't recharge via
it's attached solar panel and therefore shuts off once the battery voltage is to
low for further LoRa transmissions. The node need manual inspection and
probably a solar panel replacement.

![](/img/grafana_rssi.png)

The graph above shows shows the varying values of RSSI, which describes the
connection quality. For LoRa values down to **-120dB** are fine for a stable
connection, so the node `rain-box-1` should have a stable connection.

[raspbian]: https://www.raspberrypi.org/downloads/raspbian/
[grafana-debian]: https://grafana.com/docs/grafana/latest/installation/debian/
