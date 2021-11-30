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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <Stream.h>
#include <stdlib.h> 
#include <vector>
#include <string>

#include <default_config.h>
#include <firmware_config.h>
#include <hardware_config.h>
#include "version.h"

#define CONFIG_MAX_LINE 256
#define CONFIG_MAX_SECTION_LEN 32

/*
 * SPIFFS
 */
class DataFS {

public:
  static bool mount(bool format_on_fail = true);
  static void unmount();

  static size_t getUsedBytes();
  static size_t getTotalBytes();
};

/*
 * Config File
 */
class ConfigFile {
  public:
    typedef std::vector<std::string> StringList_t;

  public:
    ConfigFile(const char * filename = NULL);

    bool load(const char * filename);
    void print(Stream & stream, const char * label=NULL);

    /* accessors */
    const char *  get(const char * key, const char * def = NULL);
    bool          has(const char * key);
    
    int           getInt  (const char * key, int def = 0);
    unsigned int  getUInt (const char * key, unsigned int def = 0);
    long          getLong (const char * key, long def = 0);
    unsigned long getULong(const char * key, unsigned long def = 0);
    float         getFloat(const char * key, float def = 0);
    bool          getBool (const char * key, bool def = false);

    unsigned int countValues();
    unsigned int countSections();

    const StringList_t & keys();
    const StringList_t & values();
    const StringList_t & sections();
  protected:
    StringList_t _keys;
    StringList_t _values;
    StringList_t _sections;
};

/*
 * Main configuration
 */
class ConfigMain : public ConfigFile {

  public:
    enum MqttTopics {
      MQTT_TOPIC_ROOT,
      MQTT_TOPIC_AVAILABILITY,
      MQTT_TOPIC_COMMAND,
      MQTT_TOPIC_DHT_SENSOR,
      MQTT_TOPIC_WIFI,
      MQTT_TOPIC_FLORA,
      MQTT_TOPIC_LIGHT
    };

  public:

    ConfigMain();
    bool load();

    void formatTopic(
        std::string & str, MqttTopics topic, 
        const char * subTopic1 = NULL, const char * subTopic2 = NULL);

  public:

    /* WIFI settings */
    const char * wifi_ssid;
    const char * wifi_password;

    /* Station settings */
    const char * station_name;
    const char * station_root_topic;
    const char * station_availability_topic;
    const char * station_command_topic;
    const char * station_payload_offline;
    const char * station_payload_online;

    /* MQTT settings */
    const char * mqtt_host;
    uint16_t     mqtt_port;
    const char * mqtt_clientid;
    const char * mqtt_username;
    const char * mqtt_password;

    /* Display settings */
    uint16_t     display_refresh_sec;
    uint16_t     display_cycle_pages_sec;
    uint16_t     display_cycle_pages_start_sec;
    uint16_t     display_backlight_off_sec;

    /* DHT settings */
    const char * dht_base_topic;
    uint16_t     dht_publish_min_interval_sec;
    bool         dht_mqtt_retain;

    /* FLORA settings */
    const char * flora_base_topic;
    uint16_t     flora_publish_min_interval_sec;
    bool         flora_mqtt_collaborate;
    bool         flora_mqtt_retain;
    bool         flora_discover_devices;

    /* Home assistant */
    const char * hass_discovery_topic_prefix;
    const char * hass_central_device_id;
    const char * hass_station_device_id;
    const char * hass_station_suggested_area;
    bool         hass_publish_central_device;
    bool         hass_mqtt_retain;


    /* BLE settings */
    uint32_t     ble_scan_duration_sec;
    uint32_t     ble_scan_wait_sec;
    uint16_t     ble_scan_interval_ms;
    uint16_t     ble_window_interval_ms;
    bool         ble_verbose;
    bool         ble_active_scan;
};

/* inlines */
inline bool ConfigFile::has(const char * key) {
  return get(key) != NULL;
}

inline int ConfigFile::getInt(const char * key, int def) {
  return (int) getLong(key, (long) def);
}

inline unsigned int ConfigFile::getUInt(const char * key, unsigned int def) {
  return (unsigned int) getULong(key, (unsigned long) def);
}

inline long ConfigFile::getLong(const char * key, long def) {
  const char * val = get(key);
  if (val == NULL) return def;
  return strtol(val, NULL, 0);
}

inline unsigned long ConfigFile::getULong(const char * key, unsigned long def) {
  const char * val = get(key);
  if (val == NULL) return def;
  return strtoul(val, NULL, 0);
}

inline float ConfigFile::getFloat(const char * key, float def) {
  const char * val = get(key);
  if (val == NULL) return def;
  return strtof(val, NULL);
}

inline bool ConfigFile::getBool(const char * key, bool def) {
  const char * val = get(key);
  if (val == NULL) return def;
  if (strcasecmp(val, "true") == 0) return true;
  if (strcasecmp(val, "false") == 0) return false;
  return getInt(val, def ? 1:0) != 0;
}

inline unsigned int ConfigFile::countValues() {
  return (unsigned int) _keys.size();
}

inline unsigned int ConfigFile::countSections() {
  return (unsigned int) _sections.size();
}

inline const ConfigFile::StringList_t & ConfigFile::keys() {
  return _keys;
}
inline const ConfigFile::StringList_t & ConfigFile::values() {
  return _values;
}
inline const ConfigFile::StringList_t & ConfigFile::sections() {
  return _sections;
}

extern ConfigMain config;

#endif//_CONFIG_H_
