#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"

#if defined(ESP32)
#include "mbedtls/gcm.h"
#endif

static const char* ESPDM_VERSION = "0.1.0";
static const char* TAG = "smartmeter";

namespace esphome
{
    namespace smartmeter
    {
        class DlmsMeter : public Component, public uart::UARTDevice
        {
            public:
                void setup() override;
                void loop() override;
                void dump_config() override;

                void set_voltage_l1_sensor(sensor::Sensor *voltage_l1) { this->voltage_l1 = voltage_l1; }
                void set_voltage_l2_sensor(sensor::Sensor *voltage_l2) { this->voltage_l2 = voltage_l2; }
                void set_voltage_l3_sensor(sensor::Sensor *voltage_l3) { this->voltage_l3 = voltage_l3; }
                void set_current_l1_sensor(sensor::Sensor *current_l1) { this->current_l1 = current_l1; }
                void set_current_l2_sensor(sensor::Sensor *current_l2) { this->current_l2 = current_l2; }
                void set_current_l3_sensor(sensor::Sensor *current_l3) { this->current_l3 = current_l3; }
                void set_power_consumption_sensor(sensor::Sensor *power_consumption) { this->power_consumption = power_consumption; }
                void set_power_production_sensor(sensor::Sensor *power_production) { this->power_production = power_production; }
                void set_power_factor_sensor(sensor::Sensor *power_factor) { this->power_factor = power_factor; }
                void set_energy_consumption_sensor(sensor::Sensor *energy_consumption) { this->energy_consumption = energy_consumption; }
                void set_energy_production_sensor(sensor::Sensor *energy_production) { this->energy_production = energy_production; }
                void set_reactive_energy_consumption_sensor(sensor::Sensor *reactive_energy_consumption) { this->reactive_energy_consumption = reactive_energy_consumption; }
                void set_reactive_energy_production_sensor(sensor::Sensor *reactive_energy_production) { this->reactive_energy_production = reactive_energy_production; }
                void set_timestamp_sensor(text_sensor::TextSensor *timestamp) { this->timestamp = timestamp; }
                void set_meter_number_sensor(text_sensor::TextSensor *meternumber){ this->meternumber = meternumber; }

                void set_key(const std::string &key);

            private:
                int receiveBufferIndex = 0; // Current position of the receive buffer
                const static int receiveBufferSize = 1024; // Size of the receive buffer
                unsigned char receiveBuffer[receiveBufferSize]; // Stores the packet currently being received
                unsigned long lastRead = 0; // Timestamp when data was last read
                int readTimeout = 1000; // Time to wait after last byte before considering data complete

                unsigned char key[16]; // Stores the decryption key
                size_t keyLength; // Stores the decryption key length (usually 16 bytes)

#if defined(ESP32)
                mbedtls_gcm_context aes; // AES context used for decryption
#endif

                sensor::Sensor *voltage_l1 {nullptr}; // Voltage L1
                sensor::Sensor *voltage_l2 {nullptr}; // Voltage L2
                sensor::Sensor *voltage_l3 {nullptr}; // Voltage L3
                sensor::Sensor *current_l1 {nullptr}; // Current L1
                sensor::Sensor *current_l2 {nullptr}; // Current L2
                sensor::Sensor *current_l3 {nullptr}; // Current L3
                sensor::Sensor *power_consumption {nullptr}; // Active power taken from grid
                sensor::Sensor *power_production {nullptr}; // Active power put into grid
                sensor::Sensor *power_factor {nullptr}; // Power Factor
                sensor::Sensor *energy_consumption {nullptr}; // Active energy taken from grid
                sensor::Sensor *energy_production {nullptr}; // Active energy put into grid
                sensor::Sensor *reactive_energy_consumption {nullptr}; // Reactive energy taken from grid
                sensor::Sensor *reactive_energy_production {nullptr}; // Reactive energy put into grid
                text_sensor::TextSensor *timestamp {nullptr}; // Text sensor for the timestamp value
                text_sensor::TextSensor *meternumber {nullptr}; // Text sensor for the meterNumber value

                uint16_t swap_uint16(uint16_t val);
                uint32_t swap_uint32(uint32_t val);
                void log_packet(unsigned char array[], size_t length);
                void abort();
        };
    }
}
