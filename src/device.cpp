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

#include "device.h"
#include "mqtt.h"

#define LOG_TAG LOG_TAG_FLORA
#include "log.h"

MiFloraFleet fleet;

/*
 * Device implementation for MiFlora
 */ 
MiFloraDevice::MiFloraDevice(const std::string & addr): 
    conductivity (this, ATTR_ID_MOISTURE    , "Cond"), 
    temperature  (this, ATTR_ID_TEMPERATURE , "Temp"),
    moisture     (this, ATTR_ID_MOISTURE    , "Moist"), 
    illuminance  (this, ATTR_ID_ILLUMINANCE , "Light"),
    RSSI         (this, ATTR_ID_RSSI        , "RSSI"),
    _last_updated(millis()),
    _id(0),
    _address     (addr), _name("") {


  // MQTT collaboration
  if (config.flora_mqtt_collaborate) {
    std::string topic;
    
    // subscribe for getting attribute update from other stations
    config.formatTopic(topic, ConfigMain::MQTT_TOPIC_FLORA, _address.c_str(), "temp");
    mqtt.subscribeTo(topic.c_str(), s_onMQTTMessage, this);

    config.formatTopic(topic, ConfigMain::MQTT_TOPIC_FLORA, _address.c_str(), "conductivity");
    mqtt.subscribeTo(topic.c_str(), s_onMQTTMessage, this);

    config.formatTopic(topic, ConfigMain::MQTT_TOPIC_FLORA, _address.c_str(), "light");
    mqtt.subscribeTo(topic.c_str(), s_onMQTTMessage, this);

    config.formatTopic(topic, ConfigMain::MQTT_TOPIC_FLORA, _address.c_str(), "moisture");
    mqtt.subscribeTo(topic.c_str(), s_onMQTTMessage, this);
  }
}

/* update device attributes from BLE scan result */
void MiFloraDevice::updateFromBLEScan(XiaomiParseResult & result) {

  unsigned long now = millis();
  char value[16];
  std::string topic;

  // TEMPERATURE
  if (result.has_temperature) {

    if ((now - temperature.lastUpdated())/1000 > config.flora_publish_min_interval_sec) {
      sprintf(value, "%.2f", result.temperature);

      config.formatTopic(topic, ConfigMain::MQTT_TOPIC_FLORA, _address.c_str(), "temp");
      mqtt.publish(topic.c_str(), value, config.flora_mqtt_retain);
    }

    LOG_F("From BLE %s %s->%.2f", 
      _name.c_str(), temperature.getLabel(), result.temperature );

    temperature.set(result.temperature, SOURCE_BLE);
  }

  // CONDUCTIVITY
  if (result.has_conductivity) {

    if ((now - conductivity.lastUpdated()) > config.flora_publish_min_interval_sec) {
      sprintf(value, "%u", (unsigned int) result.conductivity);

      config.formatTopic(topic, ConfigMain::MQTT_TOPIC_FLORA, _address.c_str(), "conductivity");
      mqtt.publish(topic.c_str(), value, config.flora_mqtt_retain);
    }
    
    LOG_F("From BLE %s %s->%d", 
      _name.c_str(), conductivity.getLabel(), (int) result.conductivity);

    conductivity.set(result.conductivity, SOURCE_BLE);
  }

  // LIGHT
  if (result.has_illuminance) {

    if ((now - illuminance.lastUpdated()) > config.flora_publish_min_interval_sec) {
      sprintf(value, "%u", (unsigned int) result.illuminance);

      config.formatTopic(topic, ConfigMain::MQTT_TOPIC_FLORA, _address.c_str(), "light");
      mqtt.publish(topic.c_str(), value, config.flora_mqtt_retain);
    }

    LOG_F("From BLE %s %s->%d", 
      _name.c_str(), illuminance.getLabel(), (int) result.illuminance);

    illuminance.set((int)result.illuminance, SOURCE_BLE);
  }
  // MOISTURE
  if (result.has_moisture) {

    if ((now - moisture.lastUpdated()) > config.flora_publish_min_interval_sec) {
      sprintf(value, "%u", (unsigned int) result.moisture);

      config.formatTopic(topic, ConfigMain::MQTT_TOPIC_FLORA, _address.c_str(), "moisture");
      mqtt.publish(topic.c_str(), value, config.flora_mqtt_retain);
    }

    LOG_F("From BLE %s %s->%d", 
      _name.c_str(), moisture.getLabel(), (int) result.moisture);
      
    moisture.set((int)result.moisture, SOURCE_BLE);
  }
}

inline void MiFloraDevice::updateRSSI(int rssi) {

  unsigned long now = millis();
  std::string topic;
  char value[16];

  // publish RSSI on MQTT
  if ((now - RSSI.lastUpdated()) > config.flora_publish_min_interval_sec) {
    sprintf(value, "%d", rssi);

    config.formatTopic(topic, ConfigMain::MQTT_TOPIC_FLORA, _address.c_str(), "rssi");
    mqtt.publish(topic.c_str(), value, config.flora_mqtt_retain);
  }

  // update attribute
  RSSI.set(rssi, SOURCE_BLE);
}

/* update device attributes via MQTT message */
void MiFloraDevice::updateFromMQTT(const char * topic, uint8_t * _payload, unsigned int len) {

  const char * attribute;
  char payload [16];

  // payload too large
  if (len - 1 > sizeof(payload)) 
    return;

  // copy payload with null terminator
  strncpy(payload, (const char *)_payload, len);
  payload[len] = '\0';
  
  // should not happen
  attribute = strrchr(topic, '/');
  if (attribute == NULL)
    return;

  auto now = millis();

  LOG_F("From MQTT %s %s->%s", _name.c_str(), attribute, payload);

  if (strcmp(attribute, "/temp") == 0) {
    if ((now - temperature.lastUpdated()) < 10000 && temperature.getSource() == SOURCE_BLE) return;
    temperature.set(strtof(payload, NULL), SOURCE_MQTT);
  } else 
  if (strcmp(attribute, "/moisture") == 0) {
    if ((now - moisture.lastUpdated()) < 10000 && moisture.getSource() == SOURCE_BLE) return;
    moisture.set(strtol(payload, NULL, 10), SOURCE_MQTT);
  } else 
  if (strcmp(attribute, "/conductivity") == 0) {
    if ((now - conductivity.lastUpdated()) < 10000 && conductivity.getSource() == SOURCE_BLE) return;
    conductivity.set(strtol(payload, NULL, 10), SOURCE_MQTT);
  } else
  if (strcmp(attribute, "/light") == 0) {
    if ((now - illuminance.lastUpdated()) < 10000 && illuminance.getSource() == SOURCE_BLE) return;
    illuminance.set(strtol(payload, NULL, 10), SOURCE_MQTT);
  } else {
    // should not happen
  }
}

/*
 * Fleet class for managing MiFlora devices
 */
#undef LOG_TAG
#define LOG_TAG LOG_TAG_FLEET

bool MiFloraFleet::begin(const char * filename) {

  ConfigFile config;
  LOG_F("Loading devices from: %s", filename);

  // load filename
  if (config.load(filename) == false) {
    LOG_LN("Failed to load devices configuration!");
    return false;
  }

  // load from configuration
  loadFromConfig(config);
  LOG_F("Loaded %d devices from '%s'", count(), filename);

  // register to BLE to get new updates
  ble.setMifloraHandler(s_BLE_MiFloraHandler);
  return true;
}

void MiFloraFleet::loadFromConfig(ConfigFile & configDevices) {

   // load devices
  for (std::string address : configDevices.sections()) {

    // create new flora device, section name is the address
    auto id = configDevices.getInt(
        (address + ":id").c_str(), _devices.size());
    auto name = configDevices.get(
        (address + ":name").c_str(), "unknown");
        
    LOG_F("Device '%s' id:%d name:%s", 
      address.c_str(), id, name);
    
    auto flora_device = new MiFloraDevice(address);
    flora_device->setID(id);
    flora_device->setName(name);

    // install limits
    const char * min_val;
    const char * max_val;

    // moisture
    min_val = configDevices.get((address + ":min_moisture").c_str());
    max_val = configDevices.get((address + ":max_moisture").c_str());
    if (min_val) flora_device->moisture.setMin(atol(min_val));
    if (max_val) flora_device->moisture.setMax(atol(max_val));

    LOG_F(" - moisture limits [%s,%s]", 
      min_val ? min_val : "n/a", max_val ? max_val : "n/a");
    
     // temperature
    min_val = configDevices.get((address + ":min_temperature").c_str());
    max_val = configDevices.get((address + ":max_temperature").c_str());
    if (min_val) flora_device->temperature.setMin(atol(min_val));
    if (max_val) flora_device->temperature.setMax(atol(max_val));

    LOG_F(" - temperature limits [%s,%s]", 
      min_val ? min_val : "n/a", max_val ? max_val : "n/a");
      
    // conductivity
    min_val = configDevices.get((address + ":min_conductivity").c_str());
    max_val = configDevices.get((address + ":max_conductivity").c_str());
    if (min_val) flora_device->conductivity.setMin(atol(min_val));
    if (max_val) flora_device->conductivity.setMax(atol(max_val));

    LOG_F(" - conductivity limits [%s,%s]", 
      min_val ? min_val : "n/a", max_val ? max_val : "n/a");

    // illuminance
    min_val = configDevices.get((address + ":min_illuminance").c_str());
    max_val = configDevices.get((address + ":max_illuminance").c_str());
    if (min_val) flora_device->illuminance.setMin(atol(min_val));
    if (max_val) flora_device->illuminance.setMax(atol(max_val));
    
    LOG_F(" - illuminance limits [%s,%s]", 
      min_val ? min_val : "n/a", max_val ? max_val : "n/a");
      
    // RSSI
    min_val = configDevices.get((address + ":min_rssi").c_str());
    if (min_val) flora_device->RSSI.setMin(atol(min_val));

    LOG_F(" - RSSI limit %s", 
      min_val ? min_val : "n/a");
    
    // add to devices
    addDevice(flora_device);
  }
}

/* notification from BLE to interpret new data scans */
void MiFloraFleet::s_BLE_MiFloraHandler(const BLE::MiFloraScanData_t & scanData) {

  XiaomiParseResult result = {};
  MiFloraDevice * flora_device = NULL;

  // parse the scan data for Xiami devices into a result structure
  parse_xiaomi_message(scanData.serviceData, result);

  // find Flora device
  flora_device = fleet.findByAddress(scanData.deviceAddress.c_str());
 
  // create new flora device if not found
  if (flora_device == NULL) {

    // ignore new devices, if configured to do so
    if (config.flora_discover_devices == false) {
      LOG_F("New flora device: %s (ignored)",
        scanData.deviceAddress.c_str());
      return;
    }

    flora_device = new MiFloraDevice(scanData.deviceAddress);
    flora_device->setID(fleet.count());
    flora_device->setName("Unknown");
    fleet.addDevice(flora_device);
    LOG_F("New flora device: %s",flora_device->getAddress().c_str());
  }

  // update device attributes from BLE data
  flora_device->updateFromBLEScan(result);
  if (scanData.deviceRSSI != BLE_NO_RSSI) {
    flora_device->updateRSSI(scanData.deviceRSSI);
  }
  
  LOG_F("BLE updated device #%d %s (%s): ", 
    flora_device->getID(),
    flora_device->getName().c_str(), 
    flora_device->getAddress().c_str());
}