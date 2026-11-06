import esphome.codegen as cg
from esphome.components import sensor, uart
import esphome.config_validation as cv
from esphome.const import (
    CONF_PM_1_0,
    CONF_PM_2_5,
    CONF_PM_10_0,
    CONF_UPDATE_INTERVAL,
    DEVICE_CLASS_PM1,
    DEVICE_CLASS_PM25,
    DEVICE_CLASS_PM10,
    ICON_BLUR,
    SCHEDULER_DONT_RUN,
    STATE_CLASS_MEASUREMENT,
    UNIT_MICROGRAMS_PER_CUBIC_METER,
)
from esphome.types import ConfigType

CODEOWNERS = ["@habbie"]
DEPENDENCIES = ["uart"]

pm1006k_ns = cg.esphome_ns.namespace("pm1006k")
PM1006KComponent = pm1006k_ns.class_(
    "PM1006KComponent", uart.UARTDevice, cg.PollingComponent
)

PM_SENSOR_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_MICROGRAMS_PER_CUBIC_METER,
    icon=ICON_BLUR,
    accuracy_decimals=0,
    state_class=STATE_CLASS_MEASUREMENT,
)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PM1006KComponent),
            cv.Optional(CONF_PM_1_0): PM_SENSOR_SCHEMA.extend(
                {cv.Optional("device_class", default=DEVICE_CLASS_PM1): cv.string}
            ),
            cv.Optional(CONF_PM_2_5): PM_SENSOR_SCHEMA.extend(
                {cv.Optional("device_class", default=DEVICE_CLASS_PM25): cv.string}
            ),
            cv.Optional(CONF_PM_10_0): PM_SENSOR_SCHEMA.extend(
                {cv.Optional("device_class", default=DEVICE_CLASS_PM10): cv.string}
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.polling_component_schema("never")),
)


def validate_interval_uart(config: ConfigType) -> None:
    interval = config.get(CONF_UPDATE_INTERVAL)
    uart.final_validate_device_schema(
        "pm1006k",
        baud_rate=9600,
        require_rx=True,
        require_tx=interval.total_milliseconds != SCHEDULER_DONT_RUN,
        data_bits=8,
        parity="NONE",
        stop_bits=1,
    )(config)


FINAL_VALIDATE_SCHEMA = validate_interval_uart


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    sensors = sensor.sub_sensors(config)
    await sensors(CONF_PM_1_0, var.set_pm_1_0_sensor)
    await sensors(CONF_PM_2_5, var.set_pm_2_5_sensor)
    await sensors(CONF_PM_10_0, var.set_pm_10_0_sensor)
