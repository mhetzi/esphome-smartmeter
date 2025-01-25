import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor, text_sensor
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_POWER_FACTOR,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_REACTIVE_POWER,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_WATT,
    UNIT_WATT_HOURS,
    UNIT_VOLT_AMPS_REACTIVE_HOURS,
    ENTITY_CATEGORY_DIAGNOSTIC
)

DEPENDENCIES = ['uart']

smartmeter_ns = cg.esphome_ns.namespace("smartmeter")
SMART_METER = smartmeter_ns.class_("DlmsMeter", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SMART_METER),
        cv.Required("key"): cv.string,
        cv.Optional("voltage_l1"): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("voltage_l2"): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("voltage_l3"): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("current_l1"): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("current_l2"): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("current_l3"): sensor.sensor_schema(
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("power_consumption"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("power_production"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_POWER,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("power_factor"): sensor.sensor_schema(
            accuracy_decimals=2,
            device_class=DEVICE_CLASS_POWER_FACTOR,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional("energy_consumption"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT_HOURS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional("energy_production"): sensor.sensor_schema(
            unit_of_measurement=UNIT_WATT_HOURS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_ENERGY,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional("reactive_energy_consumption"): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT_AMPS_REACTIVE_HOURS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_REACTIVE_POWER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional("reactive_energy_production"): sensor.sensor_schema(
            unit_of_measurement=UNIT_VOLT_AMPS_REACTIVE_HOURS,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_REACTIVE_POWER,
            state_class=STATE_CLASS_TOTAL_INCREASING,
        ),
        cv.Optional("timestamp"): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        ),
        cv.Optional("meter_number"): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        )
    }
).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_key(config["key"]))

    if "voltage_l1" in config:
        cg.add(var.set_voltage_l1_sensor(await sensor.new_sensor(config["voltage_l1"])))

    if "voltage_l2" in config:
        cg.add(var.set_voltage_l2_sensor(await sensor.new_sensor(config["voltage_l2"])))

    if "voltage_l3" in config:
        cg.add(var.set_voltage_l3_sensor(await sensor.new_sensor(config["voltage_l3"])))

    if "current_l1" in config:
        cg.add(var.set_current_l1_sensor(await sensor.new_sensor(config["current_l1"])))

    if "current_l2" in config:
        cg.add(var.set_current_l2_sensor(await sensor.new_sensor(config["current_l2"])))

    if "current_l3" in config:
        cg.add(var.set_current_l3_sensor(await sensor.new_sensor(config["current_l3"])))

    if "power_consumption" in config:
        cg.add(var.set_power_consumption_sensor(await sensor.new_sensor(config["power_consumption"])))

    if "power_production" in config:
        cg.add(var.set_power_production_sensor(await sensor.new_sensor(config["power_production"])))

    if "power_factor" in config:
        cg.add(var.set_power_factor_sensor(await sensor.new_sensor(config["power_factor"])))

    if "energy_consumption" in config:
        cg.add(var.set_energy_consumption_sensor(await sensor.new_sensor(config["energy_consumption"])))

    if "energy_production" in config:
        cg.add(var.set_energy_production_sensor(await sensor.new_sensor(config["energy_production"])))

    if "reactive_energy_consumption" in config:
        cg.add(var.set_reactive_energy_consumption_sensor(await sensor.new_sensor(config["reactive_energy_consumption"])))

    if "reactive_energy_production" in config:
        cg.add(var.set_reactive_energy_production_sensor(await sensor.new_sensor(config["reactive_energy_production"])))

    if "timestamp" in config:
        cg.add(var.set_timestamp_sensor(await text_sensor.new_text_sensor(config["timestamp"])))

    if "meter_number" in config:
        cg.add(var.set_meter_number_sensor(await text_sensor.new_text_sensor(config["meter_number"])))
