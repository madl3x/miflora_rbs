/*
 *   MIT License
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 * 
 *  Copyright (c) 2021 Alex Mircescu
 */

#include "xiaomi.h"
#include <Arduino.h>

#define serial_print(msg) Serial.print(msg);
#define serial_println(msg) Serial.println(msg);

uint32_t encode_uint32(uint8_t msb, uint8_t byte2, uint8_t byte3, uint8_t lsb) {
 return (uint32_t(msb) << 24) | (uint32_t(byte2) << 16) | (uint32_t(byte3) << 8) | uint32_t(lsb);
}

bool parse_xiaomi_value(uint8_t value_type, const uint8_t *data, uint8_t value_length, struct XiaomiParseResult &result) {
  // motion detection, 1 byte, 8-bit unsigned integer
  if ((value_type == 0x03) && (value_length == 1)) {
    result.has_motion = data[0];
  }
  // temperature, 2 bytes, 16-bit signed integer (LE), 0.1 °C
  else if ((value_type == 0x04) && (value_length == 2)) {
    const int16_t temperature = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
    result.temperature = temperature / 10.0f;
    result.has_temperature = true;
  }
  // humidity, 2 bytes, 16-bit signed integer (LE), 0.1 %
  else if ((value_type == 0x06) && (value_length == 2)) {
    const int16_t humidity = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
    result.humidity = humidity / 10.0f;
  }
  // illuminance (+ motion), 3 bytes, 24-bit unsigned integer (LE), 1 lx
  else if (((value_type == 0x07) || (value_type == 0x0F)) && (value_length == 3)) {
    const uint32_t illuminance = uint32_t(data[0]) | (uint32_t(data[1]) << 8) | (uint32_t(data[2]) << 16);
    result.illuminance = illuminance;
    result.has_illuminance = true;
    result.is_light = illuminance == 100;
    if (value_type == 0x0F)
      result.has_motion = true;
  }
  // soil moisture, 1 byte, 8-bit unsigned integer, 1 %
  else if ((value_type == 0x08) && (value_length == 1)) {
    result.moisture = data[0];
    result.has_moisture = true;
  }
  // conductivity, 2 bytes, 16-bit unsigned integer (LE), 1 µS/cm
  else if ((value_type == 0x09) && (value_length == 2)) {
    const uint16_t conductivity = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
    result.conductivity = conductivity;
    result.has_conductivity = true;
  }
  // battery, 1 byte, 8-bit unsigned integer, 1 %
  else if ((value_type == 0x0A) && (value_length == 1)) {
    result.battery_level = data[0];
    result.has_battery = true;
  }
  // temperature + humidity, 4 bytes, 16-bit signed integer (LE) each, 0.1 °C, 0.1 %
  else if ((value_type == 0x0D) && (value_length == 4)) {
    const int16_t temperature = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
    const int16_t humidity = uint16_t(data[2]) | (uint16_t(data[3]) << 8);
    result.temperature = temperature / 10.0f;
    result.humidity = humidity / 10.0f;
  }
  // formaldehyde, 2 bytes, 16-bit unsigned integer (LE), 0.01 mg / m3
  else if ((value_type == 0x10) && (value_length == 2)) {
    const uint16_t formaldehyde = uint16_t(data[0]) | (uint16_t(data[1]) << 8);
    result.formaldehyde = formaldehyde / 100.0f;
  }
  // on/off state, 1 byte, 8-bit unsigned integer
  else if ((value_type == 0x12) && (value_length == 1)) {
    result.is_active = data[0];
  }
  // mosquito tablet, 1 byte, 8-bit unsigned integer, 1 %
  else if ((value_type == 0x13) && (value_length == 1)) {
    result.tablet = data[0];
  }
  // idle time since last motion, 4 byte, 32-bit unsigned integer, 1 min
  else if ((value_type == 0x17) && (value_length == 4)) {
    const uint32_t idle_time = encode_uint32(data[3], data[2], data[1], data[0]);
    result.idle_time = idle_time / 60.0f;
    result.has_motion = !idle_time;
  } else {
    return false;
  }

  return true;
}

bool parse_xiaomi_message(const std::vector<uint8_t> &message, struct XiaomiParseResult &result) {

  result.has_data = message[0] & 0x40;
  result.has_capability = message[0] & 0x20;
  result.has_encryption = message[0] & 0x08;  // update encryption status
  if (result.has_encryption) {
    serial_println("[XIAOMI] Error: payload is encrypted, stop reading message.");
    return false;
  }
  
  if (!result.has_data) {
    serial_println("[XIAOMI] Warning: service data has no DATA flag.");
    return false;
  }

  static uint8_t last_frame_count = 0;
  if (last_frame_count == message[4]) {
    serial_print("[XIAOMI] Warning: duplicate data packet received (frame:");
    serial_print(last_frame_count);
    serial_println(")");
    result.is_duplicate = true;
    return false;
  }
  
  last_frame_count = message[4];
  result.is_duplicate = false;
  result.raw_offset = result.has_capability ? 12 : 11;

  if ((message[2] == 0x98) && (message[3] == 0x00)) {  // MiFlora
    result.type = XiaomiParseResult::TYPE_HHCCJCY01;
    result.name = "HHCCJCY01";
  } else {
    serial_println("[XIAOMI] Error: this device is not a mi-flora device");
    return false;
  }

  // Data point specs
  // Byte 0: type
  // Byte 1: fixed 0x10
  // Byte 2: length
  // Byte 3..3+len-1: data point value

  const uint8_t *payload = message.data() + result.raw_offset;
  uint8_t payload_length = message.size() - result.raw_offset;
  uint8_t payload_offset = 0;
  bool success = false;

  if (payload_length < 4) {
    serial_print("[XIAOMI] Error: payload has wrong size:");
    serial_println(payload_length);
    return false;
  }

  while (payload_length > 3) {
    if (payload[payload_offset + 1] != 0x10 && payload[payload_offset + 1] != 0x00) {
      serial_println("[XIAOMI] Warning: fixed byte not found, stop parsing residual data");
      break;
    }

    const uint8_t value_length = payload[payload_offset + 2];
    if ((value_length < 1) || (value_length > 4) || (payload_length < (3 + value_length))) {
      serial_print("[XIAOMI] warning: value has wrong size:");
      serial_println(value_length);
      break;
    }

    const uint8_t value_type = payload[payload_offset + 0];
    const uint8_t *data = &payload[payload_offset + 3];

    if (parse_xiaomi_value(value_type, data, value_length, result))
      success = true;

    payload_length -= 3 + value_length;
    payload_offset += 3 + value_length;
  }

  return success;
}
