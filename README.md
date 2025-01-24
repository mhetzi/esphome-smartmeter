# Overview

A custom component for ESPHome for reading meter data sent by the Kaifa MA309M via M-Bus.

# Supported meters

* Kaifa MA309M

# Exposed sensors

* Voltage L1
* Voltage L2
* Voltage L3
* Current L1
* Current L2
* Current L3
* Power Consumption
* Power Generation
* Energy Consumption
* Energy Generation
* Reactive Energy Consumption
* Reactive Energy Generation
* Timestamp
* Meter Number

# Requirements

* ESP8266 or ESP32-S3
* RJ11 cable
* M-Bus to UART board (e.g. https://www.mikroe.com/m-bus-slave-click)
* RJ11 breakout board **or** soldering iron with some wires

# Software installation

In your yaml:

``` yaml
external_components:
  - source:
      type: git
      url: https://github.com/mariopenzendorfer/esphome-smartmeter
    components: [ smartmeter ]

uart:
  tx_pin: GPIOx
  rx_pin: GPIOy
  baud_rate: 2400
  rx_buffer_size: 1024

sensor:

text_sensor:

smartmeter:
  key: "<SMARTMETER_KEY>"
  voltage_l1:
    name: "Spannung L1"
  voltage_l2:
    name: "Spannung L2"
  voltage_l3:
    name: "Spannung L3"
  current_l1:
    name: "Strom L1"
  current_l2:
    name: "Strom L2"
  current_l3:
    name: "Strom L3"
  power_consumption:
    name: "Verbrauchsleistung"
  power_production:
    name: "Erzeugerleistung"
  power_factor:
    name: "Leistungsfaktor"
  energy_consumption:
    name: "Energieverbrauch"
  energy_production:
    name: "Energieerzeugung"
  reactive_energy_consumption:
    name: "Blinkenergieverbrauch"
  reactive_energy_production:
    name: "Blindenergieerzeugung"
  timestamp:
    name: "Zeit"
  meter_number:
    name: "Zählernummer"

time:
  - platform: homeassistant
    id: homeassistant_time
```

Using esphome console: `esphome run smartmeter.example.yaml`

# Hardware installation

![ESP8266 Dev Board Pinout](img/esp8266.png "Pinout")

![ESP32-S3-DevKitC-1 Dev Board Pinout](img/esp32s3devkitc1.png "Pinout")

![M-Bus Slave Click](img/mbus_slave_click.png "MBUS Board")

| **ESP8266** | **ESP32-S3-DevKitC-1** | **M-Bus Board** | **RJ11** |
| ----------- | ---------------------- | --------------- | ---------------- |
| 3.3V        | 3V3                    | 3V3             |   |
| GND         | GND                    | GND             |   |
| GPIO1       | GPIO17                 | RX              |   |
| GPIO3       | GPIO18                 | TX              |   |
|             |                        | MBUS1           | 3 |
|             |                        | MBUS2           | 4 |
