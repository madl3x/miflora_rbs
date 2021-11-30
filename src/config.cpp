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

#include <vector>
#include <string>

#include <SPIFFS.h>
#include "config.h"

#define LOG_TAG LOG_TAG_CONFIG
#include "log.h"

/*
 * Globals
 */
ConfigMain config;
ConfigFile configDevices;

/*
 * Config FS
 */
bool DataFS::mount(bool format_on_fail) {
  return SPIFFS.begin(format_on_fail);
}

void DataFS::unmount() {
  SPIFFS.end();
}

size_t DataFS::getUsedBytes() {
  return SPIFFS.usedBytes();
}

size_t DataFS::getTotalBytes() {
  return SPIFFS.totalBytes();
}


/*
 * Config File
 */
static char * _trimLeft(char * str) {
  while(*str == ' ') ++ str;
  return str;
}

static void _trimRight(char * str) {
  if (*str == '\0') return;
  char * fin = str + strlen(str) - 1;
  while (*fin == ' ' && fin >= str) {
    *fin -- = '\0';
  }
}

static void _toLower(char * src) {
  while (*src != '\0') { 
     * src = tolower(*src);
     ++ src; 
  }
}

ConfigFile::ConfigFile(const char * filename) {
  if (filename) {
    load(filename);
  }
}

bool ConfigFile::load(const char * filename) {

  File file = SPIFFS.open(filename, FILE_READ);
  
  char * line_buf;        // buffer allocated for reading lines
  char * section;         // buffer allocated for storing sections
  char * line;            // pointer indicated line content trimed by whitespaces
  char * key;             // pointer to the key
  char * value;           // pointer to the value
  char   ch;              // character to used from reading from File
  int    line_start;      // boolean to indicate beggining of the line
  unsigned int line_no;   // current line number [1, N]
  unsigned int line_len;  // current line size
  std::string section_key;// complete key path, prefixed with section_name

  // check if file opened
  if(!file){
    LOG_F("could not open: %s\r\n", filename);
    return false;
  }

  // allocate line buffer
  line_buf = (char*) malloc(CONFIG_MAX_LINE + 1);
  if (line_buf == NULL) {
    LOG_LN("OOM!");
    return false;
  }
  // allocate section name
  section = (char*) malloc(CONFIG_MAX_SECTION_LEN+1);
  if (section == NULL) {
    LOG_LN("OOM!");
    free(line_buf);
    return false;
  }

  // prepare for reading
  * section = '\0';
  line_no = 0;
  
  while(file.available()) { 

    // prepare for new line
    line_len = 0;
    line_start = 1;
    ++ line_no;
    
     do {
        // read character
        ch = file.read();
        if (ch == '\n') break;

        // trim line start, no spaces at the beggining of the line
        if (line_start && ch == ' ') continue;

        // line is a comment, ignore the line, or
        // maximum buffer exceeded
        if ( (line_start && ch == ';') || (line_len == CONFIG_MAX_LINE)) {
          // read rest of the line
          while (file.available()) {
            if (file.read() == '\n') 
            break;
          }

          // reset buffer
          line_len = 0;
          break;
        }

        // add to line
        line_buf[line_len ++] = ch;
        line_start = 0;
        
     } while (file.available());

     // nothing read, ignore
     if (line_len == 0) {
      continue;
     }
     
     // add NULL terminator
     line_buf[line_len] = '\0';

     // trim line whitespaces
     line = line_buf;
     line = _trimLeft(line);
     _trimRight(line);

     // update length
     line_len = strlen(line);
     if (line_len == 0) continue;

     //
     // [SECTION]
     //
     if (*line == '[' ) {

      // check for end bracket
      if (line[line_len-1] != ']')
      {
        LOG_F("%s: no end bracket for section at line %d\r\n",
          filename, line_no);
        continue;
      }

      // eliminate brackets
      line[line_len-1] = '\0'; ++ line;

      // trim again
      line = _trimLeft(line);
      _trimRight(line);
      line_len = strlen(line);

      // check for empty section name
      if (line_len == 0) {
        LOG_F("%s: empty section at line %d\r\n",
          filename, line_no);
        continue;
      }

      // check for sections that exceed buffer length
      if (line_len > CONFIG_MAX_SECTION_LEN) {
        LOG_F("%s: section name too big at line %d\r\n",
          filename, line_no);
        continue;
      }

      // record section as lowercase
      strcpy(section, line);
      _toLower(section);
      _sections.push_back(section);
      continue;
     }

     //
     // key = value
     //

     // search for `=` separator
     value = strchr(line_buf, '=');
     if (value == NULL) {
        LOG_F("%s: invalid line %d (%s)\r\n", 
          filename, line_no, line_buf);
        continue;
     }

     // split line in key, value pair
     * value++ = '\0';
     key = line_buf;
     
     // trim spaces from key and value
     key   = _trimLeft(key);   _trimRight(key);
     value = _trimLeft(value); _trimRight(value);

     // keys are lowercase
     _toLower(key);

     // add to keys and values
     if (*section != '\0') {
       section_key = std::string(section) + ":" + key;
     } else {
       section_key = key;
     }

     if (get(section_key.c_str()) != NULL) {
       LOG_F("%s: duplicated key '%s' at line %d\r\n", 
        filename, section_key.c_str(), line_no);
       continue;
     }

     _keys.push_back(section_key);
     _values.push_back(value);     
  }

  // cleanup
  free(line_buf);
  free(section);
  file.close();
  return true;
}

void ConfigFile::print(Stream & stream, const char * label) {

  // print label
  if (label != NULL) {
    stream.print(label);
    stream.println(":");
  }

  // print keys and values
  for (size_t i = 0 ; i < _keys.size(); ++i ){
    stream.print(" - ");
    stream.print(_keys[i].c_str());
    stream.print(" = '");
    stream.print(_values[i].c_str());
    stream.println("'");
  }
}

const char * ConfigFile::get(const char * key, const char * def) {

  // search keys
  for (size_t i = 0 ; i < _keys.size(); ++i ){
    if (_keys[i].compare(key) == 0) {
      return _values[i].c_str();
    }
  }

  // not found
  return def;
}

/*
 * Main configuration
 */
ConfigMain::ConfigMain() {
}

void ConfigMain::formatTopic(std::string & ret, MqttTopics topic, const char * subTopic1, const char * subTopic2) {

  switch (topic) {
    // just get root topic
    case MQTT_TOPIC_ROOT: {
      ret = config.station_root_topic;
    } break;

    // get availability topic
    case MQTT_TOPIC_AVAILABILITY: {
      if (station_availability_topic == NULL) {
        ret.assign(station_root_topic);
        ret.append("/station/");
        ret.append(station_name);
        ret.append("/status");
      } else {
        ret.assign(station_availability_topic);
      }
    } break;

    // get command topic
    case MQTT_TOPIC_COMMAND: {
      if (station_command_topic == NULL) {
        ret.assign(station_root_topic);
        ret.append("/station/");
        ret.append(station_name);
        ret.append("/command/");
        ret.append(subTopic1);
      } else {
        ret.assign(station_command_topic);
        ret.append("/");
        ret.append(subTopic1);
      }
    } break;

    // get dht sensor base topic
    case MQTT_TOPIC_DHT_SENSOR: {
      if (config.dht_base_topic == NULL) {
        ret.assign(station_root_topic);
        ret.append("/station/");
        ret.append(station_name);
        ret.append("/dht/");
        ret.append(subTopic1);
      } else {
        ret.assign(dht_base_topic);
        ret.append("/");
        ret.append(subTopic1);
      }
    } break;

    // Wifi signal topic
    case MQTT_TOPIC_WIFI: {
      ret.assign(station_root_topic);
      ret.append("/station/");
      ret.append(station_name);
      ret.append("/wifi/");
      ret.append(subTopic1);
    } break;

    // get flora topic
    case MQTT_TOPIC_FLORA : {
      if (flora_base_topic == NULL) {
        ret.assign(station_root_topic);
      } else {
        ret.assign(flora_base_topic);
      }
        ret.append("/");
        ret.append(subTopic1);
        ret.append("/");
        ret.append(subTopic2);
    } break;

    // get light topics
    case MQTT_TOPIC_LIGHT: {
        ret.assign(station_root_topic);
        ret.append("/station/");
        ret.append(station_name);
        ret.append("/light/");
        ret.append(subTopic1);
    } break;
  }

  return ;
}

bool ConfigMain::load() {

  bool ret = ConfigFile::load("/config.cfg");
  if (ret == false) {
    LOG_LN("Failed to load config");

    // reset everything
    _keys.clear();
    _values.clear();
    _sections.clear();

    // default values will be loaded now
  }

  // Station options
  station_name                   = get("station:name", STATION_NAME);
  station_root_topic             = get("station:root_topic", STATION_ROOT_TOPIC);

  // get availability topic
  station_availability_topic     = get("station:availability_topic", STATION_AVAILABILITY_TOPIC);
  station_payload_offline        = get("station:payload_offline", STATION_AVAILABILITY_PAYLOAD_OFF);
  station_payload_online         = get("station:payload_online", STATION_AVAILABILITY_PAYLOAD_ON);

  // get command topic
  station_command_topic          = get("station:command_topic", STATION_COMMAND_TOPIC);

  // MQTT
  mqtt_host                      = get("mqtt:host", MQTT_HOST);
  mqtt_port                      = getUInt("mqtt:port", MQTT_PORT);
  mqtt_clientid                  = get("mqtt:clientid", MQTT_CLIENTID);
  mqtt_username                  = get("mqtt:username", MQTT_USERNAME);
  mqtt_password                  = get("mqtt:password", MQTT_PASSWORD);

  // convert empty credentials to NULL
  if (mqtt_username != NULL && *mqtt_username == '\0') mqtt_username = NULL;
  if (mqtt_password != NULL && *mqtt_password == '\0') mqtt_password = NULL;

  // WIFI
  wifi_ssid                      = get("wifi:ssid", WIFI_SSID);
  wifi_password                  = get("wifi:password", WIFI_PASSWORD);

  // DISPLAY
  display_refresh_sec            = getUInt("display:refresh_sec", DISPLAY_REFRESH_SEC);
  display_cycle_pages_sec        = getUInt("display:cycle_pages_sec", DISPLAY_CYCLE_PAGES_SEC);
  display_cycle_pages_start_sec  = getUInt("display:cycle_pages_start_sec", DISPLAY_CYCLE_PAGES_START_SEC);
  display_backlight_off_sec      = getUInt("display:backlight_off_sec", DISPLAY_BACKLIGHT_OFF_SEC);

  // DHT
  dht_base_topic                 = get("dht:base_topic", DHT_BASE_TOPIC);
  dht_publish_min_interval_sec   = getUInt("dht:publish_min_interval_sec", DHT_PUBLISH_MIN_INTERVAL_SEC);
  dht_mqtt_retain                = getBool("dht:mqtt_retain", DHT_MQTT_RETAIN);

  // FLORA
  flora_base_topic               = get("flora:base_topic", FLORA_BASE_TOPIC);
  flora_publish_min_interval_sec = getUInt("flora:publish_min_interval_sec", FLORA_PUBLISH_MIN_INTERVAL_SEC);
  flora_mqtt_collaborate         = getBool("flora:mqtt_collaborate", FLORA_MQTT_COLLABORATE);
  flora_mqtt_retain              = getBool("flora:mqtt_retain", FLORA_MQTT_RETAIN);
  flora_discover_devices         = getBool("flora:discover_devices", FLORA_DISCOVER_DEVICES);

  // Home assistant
  hass_discovery_topic_prefix    = get("homeassistant:discovery_topic_prefix", HASS_DISCOVERY_TOPIC_PREFIX);
  hass_central_device_id         = get("homeassistant:central_device_id", HASS_CENTRAL_DEVICE_ID);
  hass_station_device_id         = get("homeassistant:station_device_id", HASS_STATION_DEVICE_ID);
  hass_station_suggested_area    = get("homeassistant:station_suggested_area", HASS_STATION_SUGGESTED_AREA);
  hass_publish_central_device    = getBool("homeassistant:publish_central_device", HASS_PUBLISH_CENTRAL_DEVICE);
  hass_mqtt_retain               = getBool("homeassistant:mqtt_retain", HASS_MQTT_RETAIN);

  // BLE settings
  ble_scan_duration_sec          = getUInt("ble:scan_duration_sec", BLE_SCAN_DURATION_SEC);
  ble_scan_wait_sec              = getUInt("ble:scan_wait_sec", BLE_SCAN_WAIT_SEC);
  ble_scan_interval_ms           = getUInt("ble:scan_interval_ms", BLE_SCAN_INTERVAL_MS);
  ble_window_interval_ms         = getUInt("ble:window_interval_ms", BLE_WINDOW_INTERVAL_MS);
  ble_active_scan                = getBool("ble:active_scan", BLE_ACTIVE_SCAN ? true : false);
  ble_verbose                    = getBool("ble:verbose", BLE_VERBOSE ? true : false);

  return ret;
}

