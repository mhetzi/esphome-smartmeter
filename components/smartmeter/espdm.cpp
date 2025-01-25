#include "espdm.h"
#include "espdm_mbus.h"
#include "espdm_dlms.h"
#include "espdm_obis.h"
#if defined(ESP8266)
#include <bearssl/bearssl.h>
#endif

namespace esphome
{
    namespace smartmeter
    {
        void DlmsMeter::setup()
        {
            ESP_LOGI(TAG, "DLMS smart meter component v%s started", ESPDM_VERSION);
        }

        void DlmsMeter::loop()
        {
            unsigned long currentTime = millis();

            while (this->available()) // Read while data is available
            {
                if (receiveBufferIndex >= receiveBufferSize)
                {
                    ESP_LOGE(TAG, "Buffer overflow");
                    receiveBufferIndex = 0;
                }

                receiveBuffer[receiveBufferIndex] = this->read();
                receiveBufferIndex++;

                lastRead = currentTime;

                delay(10);
            }

            if (receiveBufferIndex > 0 && currentTime - lastRead > readTimeout)
            {
                ESP_LOGV(TAG, "receiveBufferIndex: %d", receiveBufferIndex);
                if (receiveBufferIndex < 256)
                {
                    ESP_LOGE(TAG, "Received packet with invalid size %d", receiveBufferIndex);
                    return abort();
                }

                ESP_LOGD(TAG, "Handling packet");
                log_packet(receiveBuffer, receiveBufferIndex);

                // Decrypting

                uint16_t payloadLength;
                // Check if received data is enough for the given frame length
                memcpy(&payloadLength, &receiveBuffer[18], 2); // Copy payload length
                // payloadLength = swap_uint16(payloadLength) - 5;
                payloadLength = 243;

                if (receiveBufferIndex <= payloadLength)
                {
                    ESP_LOGD(TAG, "receiveBufferIndex: %d, payloadLength: %d", receiveBufferIndex, payloadLength);
                    ESP_LOGE(TAG, "Payload length is too big for received data");
                    return abort();
                }

                /*

                        payloadLengthPacket1 = swap_uint16(payloadLengthPacket1);

                        if(payloadLengthPacket1 >= payloadLength)
                            {

                            return abort();
                        }
                */

                uint16_t payloadLength1 = 228; // TODO: Read payload length 1 from data

                uint16_t payloadLength2 = payloadLength - payloadLength1;

                if (payloadLength2 >= receiveBufferIndex - DLMS_HEADER2_OFFSET - DLMS_HEADER2_LENGTH)
                {
                    ESP_LOGD(TAG, "receiveBufferIndex: %d, payloadLength2: %d", receiveBufferIndex, payloadLength2);
                    ESP_LOGE(TAG, "Payload length 2 is too big");
                    return abort();
                }

                unsigned char iv[12]; // Reserve space for the IV, always 12 chars

                memcpy(&iv[0], &receiveBuffer[DLMS_SYST_OFFSET], DLMS_SYST_LENGTH); // Copy system title to IV
                memcpy(&iv[8], &receiveBuffer[DLMS_IC_OFFSET], DLMS_IC_LENGTH);     // Copy invocation counter to IV

                unsigned char ciphertext[payloadLength];
                memcpy(&ciphertext[0], &receiveBuffer[DLMS_HEADER1_OFFSET + DLMS_HEADER1_LENGTH], payloadLength1);
                memcpy(&ciphertext[payloadLength1], &receiveBuffer[DLMS_HEADER2_OFFSET + DLMS_HEADER2_LENGTH], payloadLength2);

                unsigned char plaintext[payloadLength];
#if defined(ESP8266)
                memcpy(plaintext, ciphertext, payloadLength);
                br_gcm_context gcmCtx;
                br_aes_ct_ctr_keys bc;
                br_aes_ct_ctr_init(&bc, key, keyLength);
                br_gcm_init(&gcmCtx, &bc.vtable, br_ghash_ctmul32);
                br_gcm_reset(&gcmCtx, iv, sizeof(iv));
                br_gcm_flip(&gcmCtx);
                br_gcm_run(&gcmCtx, 0, plaintext, payloadLength);
#elif defined(ESP32)
                mbedtls_gcm_init(&aes);
                mbedtls_gcm_setkey(&aes, MBEDTLS_CIPHER_ID_AES, key, keyLength * 8);
                mbedtls_gcm_auth_decrypt(&aes, payloadLength, iv, sizeof(iv), NULL, 0, NULL, 0, ciphertext, plaintext);
                mbedtls_gcm_free(&aes);
#else
#error "Invalid Platform"
#endif

                if (plaintext[0] != 0x0F || plaintext[5] != 0x0C)
                {
                    ESP_LOGE(TAG, "OBIS: Packet was decrypted but data is invalid");
                    return abort();
                }

                // Decoding

                ESP_LOGV(TAG, "Decoding payload");
                log_packet(plaintext, 300);
                int currentPosition = DECODER_START_OFFSET;

                do
                {
                    if (plaintext[currentPosition + OBIS_TYPE_OFFSET] != DataType::OctetString)
                    {
                        ESP_LOGE(TAG, "OBIS: Unsupported OBIS header type");
                        return abort();
                    }

                    char obisCodeLength = plaintext[currentPosition + OBIS_LENGTH_OFFSET];

                    if (obisCodeLength != 0x06 && obisCodeLength != 0x0C)
                    {
                        ESP_LOGE(TAG, "OBIS: Unsupported OBIS header length");
                        return abort();
                    }

                    char obisCode[obisCodeLength];
                    memcpy(&obisCode[0], &plaintext[currentPosition + OBIS_CODE_OFFSET], obisCodeLength); // Copy OBIS code to array

                    bool timestampFound = false;
                    bool meterNumberFound = false;
                    // Do not advance Position when reading the Timestamp at DECODER_START_OFFSET
                    if ((obisCodeLength == 0x0C) && (currentPosition == DECODER_START_OFFSET))
                    {
                        timestampFound = true;
                    }
                    else if ((currentPosition != DECODER_START_OFFSET) && plaintext[currentPosition - 1] == 0xFF)
                    {
                        meterNumberFound = true;
                    }
                    else
                    {
                        currentPosition += obisCodeLength + 2; // Advance past code and position
                    }

                    char dataType = plaintext[currentPosition];
                    currentPosition++; // Advance past data type

                    char dataLength = 0x00;

                    CodeType codeType = CodeType::Unknown;

                    if (obisCode[OBIS_A] == Medium::Electricity)
                    {
                        // Compare C and D against code
                        if (memcmp(&obisCode[OBIS_C], ESPDM_VOLTAGE_L1, 2) == 0)
                        {
                            codeType = CodeType::VoltageL1;
                        }
                        else if (memcmp(&obisCode[OBIS_C], ESPDM_VOLTAGE_L2, 2) == 0)
                        {
                            codeType = CodeType::VoltageL2;
                        }
                        else if (memcmp(&obisCode[OBIS_C], ESPDM_VOLTAGE_L3, 2) == 0)
                        {
                            codeType = CodeType::VoltageL3;
                        }

                        else if (memcmp(&obisCode[OBIS_C], ESPDM_CURRENT_L1, 2) == 0)
                        {
                            codeType = CodeType::CurrentL1;
                        }
                        else if (memcmp(&obisCode[OBIS_C], ESPDM_CURRENT_L2, 2) == 0)
                        {
                            codeType = CodeType::CurrentL2;
                        }
                        else if (memcmp(&obisCode[OBIS_C], ESPDM_CURRENT_L3, 2) == 0)
                        {
                            codeType = CodeType::CurrentL3;
                        }

                        else if (memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_POWER_PLUS, 2) == 0)
                        {
                            codeType = CodeType::ActivePowerPlus;
                        }
                        else if (memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_POWER_MINUS, 2) == 0)
                        {
                            codeType = CodeType::ActivePowerMinus;
                        }
                        else if (memcmp(&obisCode[OBIS_C], ESPDM_POWER_FACTOR, 2) == 0)
                        {
                            codeType = CodeType::PowerFactor;
                        }

                        else if (memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_ENERGY_PLUS, 2) == 0)
                        {
                            codeType = CodeType::ActiveEnergyPlus;
                        }
                        else if (memcmp(&obisCode[OBIS_C], ESPDM_ACTIVE_ENERGY_MINUS, 2) == 0)
                        {
                            codeType = CodeType::ActiveEnergyMinus;
                        }

                        else if (memcmp(&obisCode[OBIS_C], ESPDM_REACTIVE_ENERGY_PLUS, 2) == 0)
                        {
                            codeType = CodeType::ReactiveEnergyPlus;
                        }
                        else if (memcmp(&obisCode[OBIS_C], ESPDM_REACTIVE_ENERGY_MINUS, 2) == 0)
                        {
                            codeType = CodeType::ReactiveEnergyMinus;
                        }
                        else
                        {
                            ESP_LOGW(TAG, "OBIS: Unsupported OBIS code");
                        }
                    }
                    else if (obisCode[OBIS_A] == Medium::Abstract)
                    {
                        if (memcmp(&obisCode[OBIS_C], ESPDM_TIMESTAMP, 2) == 0)
                        {
                            codeType = CodeType::Timestamp;
                        }
                        else if (memcmp(&obisCode[OBIS_C], ESPDM_SERIAL_NUMBER, 2) == 0)
                        {
                            codeType = CodeType::SerialNumber;
                        }
                        else if (memcmp(&obisCode[OBIS_C], ESPDM_DEVICE_NAME, 2) == 0)
                        {
                            codeType = CodeType::DeviceName;
                        }
                        else
                        {
                            ESP_LOGW(TAG, "OBIS: Unsupported OBIS code");
                        }
                    }
                    // Needed so the Timestamp at DECODER_START_OFFSET gets read correctly
                    // as it doesn't have an obisMedium
                    else if (timestampFound == true)
                    {
                        ESP_LOGV(TAG, "Found Timestamp without obisMedium");
                        codeType = CodeType::Timestamp;
                    }
                    else if (meterNumberFound == true)
                    {
                        ESP_LOGV(TAG, "Found MeterNumber without obisMedium");
                        codeType = CodeType::MeterNumber;
                    }
                    else
                    {
                        ESP_LOGE(TAG, "OBIS: Unsupported OBIS medium");
                        return abort();
                    }

                    uint8_t uint8Value;
                    uint16_t uint16Value;
                    uint32_t uint32Value;
                    float floatValue;

                    switch (dataType)
                    {
                    case DataType::DoubleLongUnsigned:
                        dataLength = 4;

                        memcpy(&uint32Value, &plaintext[currentPosition], 4); // Copy bytes to integer
                        uint32Value = swap_uint32(uint32Value);               // Swap bytes

                        floatValue = uint32Value; // Ignore decimal digits for now

                        if (codeType == CodeType::ActivePowerPlus && this->power_consumption != NULL && this->power_consumption->state != floatValue)
                            this->power_consumption->publish_state(floatValue);
                        else if (codeType == CodeType::ActivePowerMinus && this->power_production != NULL && this->power_production->state != floatValue)
                            this->power_production->publish_state(floatValue);

                        else if (codeType == CodeType::ActiveEnergyPlus && this->energy_consumption != NULL && this->energy_consumption->state != floatValue)
                            this->energy_consumption->publish_state(floatValue);
                        else if (codeType == CodeType::ActiveEnergyMinus && this->energy_production != NULL && this->energy_production->state != floatValue)
                            this->energy_production->publish_state(floatValue);

                        else if (codeType == CodeType::ReactiveEnergyPlus && this->reactive_energy_consumption != NULL && this->reactive_energy_consumption->state != floatValue)
                            this->reactive_energy_consumption->publish_state(floatValue);
                        else if (codeType == CodeType::ReactiveEnergyMinus && this->reactive_energy_production != NULL && this->reactive_energy_production->state != floatValue)
                            this->reactive_energy_production->publish_state(floatValue);

                        break;

                    case DataType::LongUnsigned:
                        dataLength = 2;

                        memcpy(&uint16Value, &plaintext[currentPosition], 2); // Copy bytes to integer
                        uint16Value = swap_uint16(uint16Value);               // Swap bytes

                        if (plaintext[currentPosition + 5] == Accuracy::SingleDigit)
                            floatValue = uint16Value / 10.0; // Divide by 10 to get decimal places
                        else if (plaintext[currentPosition + 5] == Accuracy::DoubleDigit)
                            floatValue = uint16Value / 100.0; // Divide by 100 to get decimal places
                        else
                            floatValue = uint16Value; // No decimal places

                        if (codeType == CodeType::VoltageL1 && this->voltage_l1 != NULL && this->voltage_l1->state != floatValue)
                            this->voltage_l1->publish_state(floatValue);
                        else if (codeType == CodeType::VoltageL2 && this->voltage_l2 != NULL && this->voltage_l2->state != floatValue)
                            this->voltage_l2->publish_state(floatValue);
                        else if (codeType == CodeType::VoltageL3 && this->voltage_l3 != NULL && this->voltage_l3->state != floatValue)
                            this->voltage_l3->publish_state(floatValue);

                        else if (codeType == CodeType::CurrentL1 && this->current_l1 != NULL && this->current_l1->state != floatValue)
                            this->current_l1->publish_state(floatValue);
                        else if (codeType == CodeType::CurrentL2 && this->current_l2 != NULL && this->current_l2->state != floatValue)
                            this->current_l2->publish_state(floatValue);
                        else if (codeType == CodeType::CurrentL3 && this->current_l3 != NULL && this->current_l3->state != floatValue)
                            this->current_l3->publish_state(floatValue);

                        else if (codeType == CodeType::PowerFactor && this->power_factor != NULL && this->power_factor->state != floatValue)
                            this->power_factor->publish_state(floatValue / 1000.0);

                        break;

                    case DataType::OctetString:
                        dataLength = plaintext[currentPosition];
                        currentPosition++; // Advance past string length

                        if (codeType == CodeType::Timestamp) // Handle timestamp generation
                        {
                            char timestamp[20]; // 0000-00-00 00:00:00

                            uint16_t year;
                            uint8_t month;
                            uint8_t day;

                            uint8_t hour;
                            uint8_t minute;
                            uint8_t second;

                            memcpy(&uint16Value, &plaintext[currentPosition], 2);
                            year = swap_uint16(uint16Value);

                            memcpy(&month, &plaintext[currentPosition + 2], 1);
                            memcpy(&day, &plaintext[currentPosition + 3], 1);

                            memcpy(&hour, &plaintext[currentPosition + 5], 1);
                            memcpy(&minute, &plaintext[currentPosition + 6], 1);
                            memcpy(&second, &plaintext[currentPosition + 7], 1);

                            sprintf(timestamp, "%04u-%02u-%02u %02u:%02u:%02u", year, month, day, hour, minute, second);

                            this->timestamp->publish_state(timestamp);
                        }
                        else if (codeType == CodeType::MeterNumber)
                        {
                            ESP_LOGV(TAG, "Constructing MeterNumber...");
                            char meterNumber[13]; // 121110284568

                            memcpy(meterNumber, &plaintext[currentPosition], dataLength);
                            meterNumber[12] = '\0';

                            this->meternumber->publish_state(meterNumber);
                        }

                        break;
                    default:
                        ESP_LOGE(TAG, "OBIS: Unsupported OBIS data type");
                        return abort();
                        break;
                    }

                    currentPosition += dataLength; // Skip data length

                    // Don't skip the break on the first Timestamp, as there's none
                    if (timestampFound == false)
                    {
                        currentPosition += 2; // Skip break after data
                    }

                    if (plaintext[currentPosition] == 0x0F) // There is still additional data for this type, skip it
                    {
                        // currentPosition += 6; // Skip additional data and additional break; this will jump out of bounds on last frame
                        //  on EVN Meters the additional data (no additional Break) is only 3 Bytes + 1 Byte for the "there is data" Byte
                        currentPosition += 4; // Skip additional data and additional break; this will jump out of bounds on last frame
                    }
                } while (currentPosition <= payloadLength); // Loop until arrived at end

                receiveBufferIndex = 0;

                ESP_LOGI(TAG, "Received valid data");
            }
        }

        void DlmsMeter::abort()
        {
            receiveBufferIndex = 0;
        }

        uint16_t DlmsMeter::swap_uint16(uint16_t val)
        {
            return (val << 8) | (val >> 8);
        }

        uint32_t DlmsMeter::swap_uint32(uint32_t val)
        {
            val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
            return (val << 16) | (val >> 16);
        }

        void DlmsMeter::set_key(const std::string &key)
        {
            for (unsigned int i = 0; i < key.length() && i < sizeof(key)*2; i += 2)
            {
                std::string byteString = key.substr(i, 2);
                this->key[keyLength] = (unsigned char)strtol(byteString.c_str(), NULL, 16);
                keyLength++;
            }
        }

        void DlmsMeter::log_packet(unsigned char array[], size_t length)
        {
            char buffer[(length * 3)];

            for (unsigned int i = 0; i < length; i++)
            {
                char nib1 = (array[i] >> 4) & 0x0F;
                char nib2 = (array[i] >> 0) & 0x0F;
                buffer[i * 3] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA;
                buffer[i * 3 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA;
                buffer[i * 3 + 2] = ' ';
            }

            buffer[(length * 3) - 1] = '\0';

            ESP_LOGI(TAG, buffer);
        }
    }
}
