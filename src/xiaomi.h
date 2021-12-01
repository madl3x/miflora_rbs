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

#ifndef _XIAOMI_H_
#define _XIAOMI_H_

#include <string>
#include <vector>

/* 
 * Code modified from the implementation made by ESPHOME for the xiaomi tracker.
 */

/* Result structure from parsing service data message */
struct XiaomiParseResult {
  enum {
    TYPE_HHCCJCY01,
    TYPE_GCLS002,
    TYPE_HHCCPOT002,
    TYPE_LYWSDCGQ,
    TYPE_LYWSD02,
    TYPE_CGG1,
    TYPE_LYWSD03MMC,
    TYPE_CGD1,
    TYPE_CGDK2,
    TYPE_JQJCY01YM,
    TYPE_MUE4094RT,
    TYPE_WX08ZM,
    TYPE_MJYD02YLA,
    TYPE_MHOC401,
    TYPE_CGPR1
  } type;
  const char * name;
  float temperature;
  float humidity;
  float moisture;
  float conductivity;
  float illuminance;
  float formaldehyde;
  float battery_level;
  float tablet;
  float idle_time;
  bool is_active;
  bool has_motion;
  bool is_light;
  bool has_temperature;
  bool has_illuminance;
  bool has_conductivity;
  bool has_moisture;
  bool has_battery;
  bool has_data;        // 0x40
  bool has_capability;  // 0x20
  bool has_encryption;  // 0x08
  bool is_duplicate;    // true if the packet is duplicate
  int raw_offset;
};

/* Parses data charactestic and extracts the result */
bool parse_xiaomi_message(
  const std::vector<uint8_t> &message, 
  struct XiaomiParseResult &result );

#endif//_XIAOMI_H_
