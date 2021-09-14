# InfluxDB Database

This sections covers briefly the setup of InfluxDB, a time series database which
allows storing and querying metrics. Just like all other tools used within this
document it's open source and all [code is available online][github-influxdb].

Using InfluxDB comes with the two advantages of offering integrations for both
[Grafana](grafana.md) as well consuming MQTT events, which is the way the
[LoRaWAN brooker](thethingsnetwork.md) reports metrics.

## Installation of InfluxDB

> While InfluxDB 2.0 was released this year, this document describes the setup and
usage of InfluxDB 1.8. All tooling should be compatible with both versions and a
migration guide might be added later on.

The upstream documentation explains the installation process for various setups,
for the RaspberryPi setup the [Ubuntu  & Debian][influxdb-install] steps should
be followed. Within this documentation both database and user are called
`telegraf`, however other use cases may use other values.

## Installation of Telegraf

> Telegraf is used to consume MQTT events and store them inside InfluxDB. 

InfluxDB and Telegraf are both developed by *InfluxData Inc.*, so a similar
installation process is offered. Again the official installation guide for
[Ubuntu & Debian][telegraf-install] should be used. 

Once installed a custom configuration is required to consume data from
TheThingsNetwork and store them locally. All values in `<...>` should be changed
accordingly to the actual setup.

```toml
# /etc/telegraf/telegraf.conf
[global_tags]
[agent]
  interval = "10s"
  round_interval = true
  metric_batch_size = 1000
  metric_buffer_limit = 10000
  collection_jitter = "0s"
  flush_interval = "10s"
  flush_jitter = "0s"
  precision = ""
  hostname = ""
  omit_hostname = false

[[outputs.influxdb]]
  database = "telegraf"
  urls = [ "http://localhost:8086" ]
  username = "<influxdb_username>"
  password = "<influxdb_password>"

[[inputs.mqtt_consumer]]
  servers = ["tcp://nam1.cloud.thethings.network:1883"]                                              
  topics = [ "#" ]
  username = "<thethingsnetwork_username>"
  password = "NNSXS.<thethingsnetwork_api_token>"
  data_format = "json"
  tag_keys = [
    "end_device_ids_device_id"
  ]
```

Once both services are running one should proceed setting up
[Grafana](grafana.md) to create a dashboard presenting collected metrics.

[github-influxdb]: https://github.com/influxdata/influxdb/
[influxdb-install]: https://docs.influxdata.com/influxdb/v1.8/introduction/install/
[telegraf-install]: https://docs.influxdata.com/telegraf/v1.19/introduction/installation/
