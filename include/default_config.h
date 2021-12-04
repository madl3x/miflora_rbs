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

#ifndef _DEFAULT_CONFIG_H_
#define _DEFAULT_CONFIG_H_

/*
 * This configuration sums the default values that the firmware will use
 * when the corresponding option is not found in the "config.cfg" file.
 * They are essentially your compile-time defaults, but any of these can
 * be overwritten by the SPIFFS's "config.cfg" file. See the /data folder
 * for more details.
 */

#define WIFI_SSID                        "WIFISSID" // <--- your WIFI SSID (if you don't want this hardcoded use "config.cfg")
#define WIFI_PASSWORD                    "password" // <--- your WIFI password (if you don't want this hardcoded use "config.cfg")
#define WIFI_RESTART_CORE_SEC                  120  // inteval in seconds restart the ESP core if WiFi doesn't connect (0 to disable)

#define STATION_NAME                     "station1" // <--- the name of this station
#define STATION_ROOT_TOPIC            "miflora_rbs" // root_topic, make sure this is common if you want the stations to collaborate
#define STATION_AVAILABILITY_TOPIC             NULL // defaults to <root_topic>/station/<station_name>/status
#define STATION_AVAILABILITY_PAYLOAD_ON    "online" // default is "online" for HomeAssistant
#define STATION_AVAILABILITY_PAYLOAD_OFF  "offline" // default is "offline" for HomeAssistant
#define STATION_COMMAND_TOPIC                  NULL // defaults to <root_topic>/station/<station_name>/command

#define MQTT_HOST                     "192.168.1.1" // <--- MQTT broker IP
#define MQTT_PORT                              1883 // <--- MQTT broker port
#define MQTT_CLIENTID                          NULL // if NULL it defaults to <stationname>-<mac_addr_suffix>
#define MQTT_USERNAME                          NULL
#define MQTT_PASSWORD                          NULL

#define DISPLAY_REFRESH_SEC                       5 // interval to update the screen in seconds
#define DISPLAY_CYCLE_PAGES_SEC                  20 // interval to automatically cycle between pages and screens
#define DISPLAY_CYCLE_PAGES_START_SEC           120 // interval to delay cycling pages when a button is pressed
#define DISPLAY_BACKLIGHT_OFF_SEC                60 // interval to automatically switch backlight off

#define DHT_BASE_TOPIC                          NULL // if NULL it defaults to <root_topic>/station/<station_name>/dht
#define DHT_PUBLISH_MIN_INTERVAL_SEC              60 // don't publish DHT values more often than this
#define DHT_MQTT_RETAIN                        false // for retaining values published via MQTT

//
// For XIAOMI MiFlora devices the final MQTT topic is 
// <FLORA_BASE_TOPIC>/<device_address>/<characteristic_name> 
// where characteristic_name can be either: moisture, temp, light and conductivity
//

#define FLORA_BASE_TOPIC                         NULL // if NULL it defaults to STATION_ROOT_TOPIC (make sure this is common if you want station to collaborate)
#define FLORA_PUBLISH_MIN_INTERVAL_SEC             10 // don't publish discovered attributes sooner than this
#define FLORA_MQTT_COLLABORATE                   true // will use MQTT to collaborate for getting new values
#define FLORA_MQTT_RETAIN                        true // for retaining the values published via MQTT
#define FLORA_DISCOVER_DEVICES                   true // automatically add devices that are not configured via devices.cfg

#define BLE_SCAN_DURATION_SEC                      20 // interval in seconds to scan for BLE advertisments
#define BLE_SCAN_WAIT_SEC                          30 // interval in seconds to wait between scans
#define BLE_SCAN_INTERVAL_MS                       50 // interval in ms to perform the actual BLE scan
#define BLE_WINDOW_INTERVAL_MS                     30 // window internal in ms
#define BLE_ACTIVE_SCAN                          true // active or passive BLE scanning
#define BLE_VERBOSE                             false // for debugging BLE activity

#define HASS_DISCOVERY_TOPIC_PREFIX   "homeassistant" // discovery topic prefix configured for HASS
#define HASS_CENTRAL_DEVICE_ID       "miflora_plants" // Keep this name common to all stations (along with root_topic) to have a common device for all plants
#define HASS_STATION_DEVICE_ID                   NULL // defaults to miflora_rbs_<station_name>
#define HASS_STATION_SUGGESTED_AREA              NULL // set this to a string indicating the suggested area for the device (i.e. "living")
#define HASS_PUBLISH_CENTRAL_DEVICE              true // enables discovery of the central device (you can keep this to true only for a "master" station)
#define HASS_MQTT_RETAIN                         true // set retain for discovery message

#endif//_DEFAULT_CONFIG_H_